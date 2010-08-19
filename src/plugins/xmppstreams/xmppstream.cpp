#include "xmppstream.h"

#include <QInputDialog>

#define KEEP_ALIVE_TIMEOUT          30000


XmppStream::XmppStream(IXmppStreams *AXmppStreams, const Jid &AStreamJid) : QObject(AXmppStreams->instance())
{
	FXmppStreams = AXmppStreams;

	FOpen = false;
	FStreamJid = AStreamJid;
	FConnection = NULL;
	FStreamState = SS_OFFLINE;

	connect(&FParser,SIGNAL(opened(QDomElement)), SLOT(onParserOpened(QDomElement)));
	connect(&FParser,SIGNAL(element(QDomElement)), SLOT(onParserElement(QDomElement)));
	connect(&FParser,SIGNAL(error(const QString &)), SLOT(onParserError(const QString &)));
	connect(&FParser,SIGNAL(closed()), SLOT(onParserClosed()));

	FKeepAliveTimer.setSingleShot(false);
	connect(&FKeepAliveTimer,SIGNAL(timeout()),SLOT(onKeepAliveTimeout()));
}

XmppStream::~XmppStream()
{
	close();

	foreach(IXmppFeature *feature, FActiveFeatures.toSet())
		delete feature->instance();

	emit streamDestroyed();
}

bool XmppStream::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream == this && AOrder == XSHO_XMPP_STREAM)
	{
		if (FStreamState==SS_INITIALIZE && AStanza.element().nodeName()=="stream:stream")
		{
			FStreamId = AStanza.id();
			FStreamState = SS_FEATURES;
			if (VersionParser(AStanza.element().attribute("version","0.0")) < VersionParser(1,0))
			{
				Stanza stanza("stream:features");
				stanza.addElement("register",NS_FEATURE_REGISTER);
				stanza.addElement("auth",NS_FEATURE_IQAUTH);
				xmppStanzaIn(AXmppStream, stanza, AOrder);
			}
			return true;
		}
		else if (FStreamState==SS_FEATURES && AStanza.element().nodeName()=="stream:features")
		{
			FServerFeatures = AStanza.element().cloneNode(true).toElement();
			FAvailFeatures = FXmppStreams->xmppFeaturesOrdered();
			processFeatures();
			return true;
		}
		else if (AStanza.element().nodeName() == "stream:error")
		{
			ErrorHandler err(AStanza.element(),NS_XMPP_STREAMS);
			abort(err.message());
			return true;
		}
	}
	return false;
}

bool XmppStream::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool XmppStream::isOpen() const
{
	return FOpen;
}

bool XmppStream::open()
{
	if (FConnection && FStreamState==SS_OFFLINE)
	{
		bool hasPassword = !FPassword.isEmpty() || !FSessionPassword.isEmpty();
		if (!hasPassword)
		{
			FSessionPassword = QInputDialog::getText(NULL,tr("Password request"),tr("Enter password for <b>%1</b>").arg(FStreamJid.hBare()),
			                   QLineEdit::Password,FSessionPassword,&hasPassword,Qt::Dialog);
		}

		if (hasPassword)
		{
			if (FConnection->connectToHost())
			{
				FStreamState = SS_CONNECTING;
				return true;
			}
			else
			{
				abort(tr("Failed to start connection"));
			}
		}
	}
	else if (!FConnection)
	{
		emit error(tr("Connection not specified"));
	}
	return false;
}

void XmppStream::close()
{
	if (FConnection && FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
	{
		if (FConnection->isOpen())
		{
			emit aboutToClose();
			QByteArray data = "</stream:stream>";
			if (!processDataHandlers(data,true))
				FConnection->write(data);
		}
		FConnection->disconnectFromHost();
	}
	else
	{
		FStreamState = SS_OFFLINE;
	}
}

void XmppStream::abort(const QString &AError)
{
	if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
	{
		FStreamState = SS_ERROR;
		FErrorString = AError;
		emit error(AError);
		FConnection->disconnectFromHost();
	}
}

QString XmppStream::streamId() const
{
	return FStreamId;
}

QString XmppStream::errorString() const
{
	return FErrorString;
}

Jid XmppStream::streamJid() const
{
	return FStreamJid;
}

void XmppStream::setStreamJid(const Jid &AJid)
{
	if (FStreamJid != AJid && (FStreamState == SS_OFFLINE || (FStreamState == SS_FEATURES && (FStreamJid && AJid))))
	{
		if (FStreamState == SS_FEATURES && !FOfflineJid.isValid())
			FOfflineJid = FStreamJid;

		if (!(FStreamJid && AJid))
			FSessionPassword.clear();

		Jid before = FStreamJid;
		emit jidAboutToBeChanged(AJid);
		FStreamJid = AJid;
		emit jidChanged(before);
	}
}

QString XmppStream::password() const
{
	if (FPassword.isEmpty() && FStreamState == SS_FEATURES)
		return FSessionPassword;
	return FPassword;
}

void XmppStream::setPassword(const QString &APassword)
{
	FPassword = APassword;
}
QString XmppStream::defaultLang() const
{
	return FDefLang;
}

void XmppStream::setDefaultLang( const QString &ADefLang )
{
	FDefLang = ADefLang;
}

IConnection *XmppStream::connection() const
{
	return FConnection;
}

void XmppStream::setConnection(IConnection *AConnection)
{
	if (FStreamState == SS_OFFLINE && FConnection != AConnection)
	{
		if (FConnection)
			FConnection->instance()->disconnect(this);

		if (AConnection)
		{
			connect(AConnection->instance(),SIGNAL(connected()),SLOT(onConnectionConnected()));
			connect(AConnection->instance(),SIGNAL(readyRead(qint64)),SLOT(onConnectionReadyRead(qint64)));
			connect(AConnection->instance(),SIGNAL(error(const QString &)),SLOT(onConnectionError(const QString &)));
			connect(AConnection->instance(),SIGNAL(disconnected()),SLOT(onConnectionDisconnected()));
		}

		FConnection = AConnection;
		emit connectionChanged(AConnection);
	}
}

qint64 XmppStream::sendStanza(Stanza &AStanza)
{
	if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
	{
		if (!processStanzaHandlers(AStanza,true))
			return sendData(AStanza.toByteArray());
	}
	return -1;
}

void XmppStream::insertXmppDataHandler(IXmppDataHandler *AHandler, int AOrder)
{
	if (AHandler && !FDataHandlers.contains(AOrder, AHandler))
	{
		FDataHandlers.insertMulti(AOrder, AHandler);
		emit dataHandlerInserted(AHandler, AOrder);
	}
}

void XmppStream::removeXmppDataHandler(IXmppDataHandler *AHandler, int AOrder)
{
	if (FDataHandlers.contains(AOrder, AHandler))
	{
		FDataHandlers.remove(AOrder, AHandler);
		emit dataHandlerRemoved(AHandler, AOrder);
	}
}

void XmppStream::insertXmppStanzaHandler(IXmppStanzaHadler *AHandler, int AOrder)
{
	if (AHandler && !FStanzaHandlers.contains(AOrder, AHandler))
	{
		FStanzaHandlers.insertMulti(AOrder, AHandler);
		emit stanzaHandlerInserted(AHandler, AOrder);
	}
}

void XmppStream::removeXmppStanzaHandler(IXmppStanzaHadler *AHandler, int AOrder)
{
	if (FStanzaHandlers.contains(AOrder, AHandler))
	{
		FStanzaHandlers.remove(AOrder, AHandler);
		emit stanzaHandlerRemoved(AHandler, AOrder);
	}
}

void XmppStream::startStream()
{
	FParser.restart();
	FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);

	QDomDocument doc;
	QDomElement root = doc.createElementNS(NS_JABBER_STREAMS,"stream:stream");
	doc.appendChild(root);
	root.setAttribute("xmlns",NS_JABBER_CLIENT);
	root.setAttribute("to", FStreamJid.domain());
	if (!FDefLang.isEmpty())
		root.setAttribute("xml:lang",FDefLang);

	FStreamState = SS_INITIALIZE;
	Stanza stanza(doc.documentElement());
	if (!processStanzaHandlers(stanza,true))
	{
		QByteArray data = QString("<?xml version=\"1.0\"?>").toUtf8()+stanza.toByteArray().trimmed();
		data.remove(data.size()-2,1);
		sendData(data);
	}
}

void XmppStream::processFeatures()
{
	bool started = false;
	while (!started && !FAvailFeatures.isEmpty())
	{
		QString featureNS = FAvailFeatures.takeFirst();
		QDomElement featureElem = FServerFeatures.firstChildElement();
		while (!featureElem.isNull() && featureElem.namespaceURI()!=featureNS)
			featureElem = featureElem.nextSiblingElement();
		started = featureElem.namespaceURI()==featureNS ? startFeature(featureNS, featureElem) : false;
	}
	if (!started)
	{
		FOpen = true;
		FStreamState = SS_ONLINE;
		removeXmppStanzaHandler(this,XSHO_XMPP_STREAM);
		emit opened();
	}
}

bool XmppStream::startFeature(const QString &AFeatureNS, const QDomElement &AFeatureElem)
{
	IXmppFeature *feature = FXmppStreams->xmppFeaturePlugin(AFeatureNS)->newXmppFeature(AFeatureNS, this);
	if (feature)
	{
		FActiveFeatures.append(feature);
		connect(feature->instance(),SIGNAL(finished(bool)),SLOT(onFeatureFinished(bool)));
		connect(feature->instance(),SIGNAL(error(const QString &)),SLOT(onFeatureError(const QString &)));
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		connect(this,SIGNAL(closed()),feature->instance(),SLOT(deleteLater()));
		return feature->start(AFeatureElem);
	}
	return false;
}

bool XmppStream::processDataHandlers(QByteArray &AData, bool ADataOut)
{
	bool hooked = false;
	QMapIterator<int, IXmppDataHandler *> it(FDataHandlers);
	if (!ADataOut)
		it.toBack();
	while (!hooked && (ADataOut ? it.hasNext() : it.hasPrevious()))
	{
		if (ADataOut)
		{
			it.next();
			hooked = it.value()->xmppDataOut(this, AData, it.key());
		}
		else
		{
			it.previous();
			hooked = it.value()->xmppDataIn(this, AData, it.key());
		}
	}
	return hooked;
}

bool XmppStream::processStanzaHandlers(Stanza &AStanza, bool AElementOut)
{
	bool hooked = false;
	QMapIterator<int, IXmppStanzaHadler *> it(FStanzaHandlers);
	if (!AElementOut)
		it.toBack();
	while (!hooked && (AElementOut ? it.hasNext() : it.hasPrevious()))
	{
		if (AElementOut)
		{
			it.next();
			hooked = it.value()->xmppStanzaOut(this, AStanza, it.key());
		}
		else
		{
			it.previous();
			hooked = it.value()->xmppStanzaIn(this, AStanza, it.key());
		}
	}
	return hooked;
}

qint64 XmppStream::sendData(QByteArray AData)
{
	if (!processDataHandlers(AData,true))
	{
		FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);
		return FConnection->write(AData);
	}
	return 0;
}

QByteArray XmppStream::receiveData(qint64 ABytes)
{
	FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);
	return FConnection->read(ABytes);
}

void XmppStream::onConnectionConnected()
{
	insertXmppStanzaHandler(this,XSHO_XMPP_STREAM);
	startStream();
}

void XmppStream::onConnectionReadyRead(qint64 ABytes)
{
	QByteArray data = receiveData(ABytes);
	if (!processDataHandlers(data,false))
		if (!data.isEmpty())
			FParser.parseData(data);
}

void XmppStream::onConnectionError(const QString &AError)
{
	abort(AError);
}

void XmppStream::onConnectionDisconnected()
{
	FOpen = false;
	FKeepAliveTimer.stop();
	FStreamState = SS_OFFLINE;
	removeXmppStanzaHandler(this,XSHO_XMPP_STREAM);
	emit closed();

	if (FOfflineJid.isValid())
	{
		setStreamJid(FOfflineJid);
		FOfflineJid = Jid();
	}
}

void XmppStream::onParserOpened(QDomElement AElem)
{
	Stanza stanza(AElem);
	processStanzaHandlers(stanza,false);
}

void XmppStream::onParserElement(QDomElement AElem)
{
	Stanza stanza(AElem);
	processStanzaHandlers(stanza,false);
}

void XmppStream::onParserError(const QString &AError)
{
	abort(AError);
}

void XmppStream::onParserClosed()
{
	FConnection->disconnectFromHost();
}

void XmppStream::onFeatureFinished(bool ARestart)
{
	if (!ARestart)
		processFeatures();
	else
		startStream();
}

void XmppStream::onFeatureError(const QString &AError)
{
	FSessionPassword.clear();
	abort(AError);
}

void XmppStream::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	FActiveFeatures.removeAll(feature);
}

void XmppStream::onKeepAliveTimeout()
{
	if (FStreamState!=SS_ONLINE)
		abort(tr("XMPP connection timed out"));
	else
		sendData(" ");
}
