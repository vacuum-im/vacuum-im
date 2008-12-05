#include "xmppstream.h"

#include <QInputDialog>

#define KEEP_ALIVE_TIMEOUT          30000

XmppStream::XmppStream(IXmppStreams *AXmppStreams, const Jid &AJid) : QObject(AXmppStreams->instance())
{
  FXmppStreams = AXmppStreams;

  FOpen = false;
  FJid = AJid;
  FXmppVersion = "1.0";
  FConnection = NULL;
  FActiveFeature = NULL;
  FStreamState = SS_OFFLINE;

  connect(&FParser,SIGNAL(opened(const QDomElement &)), SLOT(onParserOpened(const QDomElement &)));
  connect(&FParser,SIGNAL(element(const QDomElement &)), SLOT(onParserElement(const QDomElement &)));
  connect(&FParser,SIGNAL(error(const QString &)), SLOT(onParserError(const QString &)));
  connect(&FParser,SIGNAL(closed()), SLOT(onParserClosed()));

  FKeepAliveTimer.setSingleShot(false);
  connect(&FKeepAliveTimer,SIGNAL(timeout()),SLOT(onKeepAliveTimeout()));
}

XmppStream::~XmppStream()
{
  close();

  QList<IStreamFeature *> features = FFeatures;
  foreach(IStreamFeature *feature, features)
    removeFeature(feature);

  emit destroyed((IXmppStream *)this);
}

void XmppStream::open()
{
  if (FConnection && FStreamState == SS_OFFLINE)
  {
    bool hasPassword = !FPassword.isEmpty() || !FSessionPassword.isEmpty();
    if (!hasPassword)
    {
      FSessionPassword = QInputDialog::getText(NULL,tr("Password request"),tr("Enter password for <b>%1</b>").arg(FJid.hBare()),
        QLineEdit::Password,FSessionPassword,&hasPassword,Qt::Dialog);
    }

    if (hasPassword)
    {
      FStreamState = SS_CONNECTING;
      FConnection->connectToHost();
    }
    else
      emit error(this,tr("Password not specified"));
  }
  else if (!FConnection)
   emit error(this, tr("Connection not specified"));
}

void XmppStream::close()
{
  if (FStreamState == SS_ONLINE)
    emit aboutToClose(this);

  if (FConnection)
  {
    if (FConnection->isOpen())
    {
      QByteArray data = "</stream:stream>";
      if (!hookFeatureData(&data,IStreamFeature::DirectionOut))
        FConnection->write(data);
    }
    FConnection->disconnect();
  }
  FStreamState = SS_OFFLINE;
}

void XmppStream::setJid(const Jid &AJid)
{
  if (FJid != AJid && (FStreamState == SS_OFFLINE || (FStreamState == SS_FEATURES && (FJid && AJid))))
  {
    if (FStreamState == SS_FEATURES && !FOfflineJid.isValid())
      FOfflineJid = FJid;

    if (!(FJid && AJid))
      FSessionPassword.clear();

    Jid befour = FJid;
    emit jidAboutToBeChanged(this, AJid);
    FJid = AJid;
    emit jidChanged(this, befour);
  }
}

const QString &XmppStream::password() const
{
  if (FPassword.isEmpty() && FStreamState == SS_FEATURES)
    return FSessionPassword;
  return FPassword;
}

qint64 XmppStream::sendStanza(const Stanza &AStanza)
{
  if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
  {
    Stanza stanza(AStanza);
    QDomElement elem = stanza.element();
    if (!hookFeatureElement(&elem,IStreamFeature::DirectionOut))
    {
      showInConsole(stanza.element(),IStreamFeature::DirectionOut);
      return sendData(stanza.toByteArray());
    }
  }
  return 0;
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
    {
      FConnection->instance()->disconnect(this);
      emit connectionRemoved(this, FConnection);
    }

    FConnection = AConnection;

    if (FConnection)
    {
      connect(FConnection->instance(),SIGNAL(connected()),SLOT(onConnectionConnected()));
      connect(FConnection->instance(),SIGNAL(readyRead(qint64)),SLOT(onConnectionReadyRead(qint64)));
      connect(FConnection->instance(),SIGNAL(disconnected()),SLOT(onConnectionDisconnected()));
      connect(FConnection->instance(),SIGNAL(error(const QString &)),SLOT(onConnectionError(const QString &)));
      emit connectionAdded(this, FConnection);
    }
  }
}

void XmppStream::addFeature(IStreamFeature *AFeature)
{
  if (AFeature && !FFeatures.contains(AFeature))
  {
    connect(AFeature->instance(),SIGNAL(ready(bool)),SLOT(onFeatureReady(bool)));
    connect(AFeature->instance(),SIGNAL(error(const QString &)),SLOT(onFeatureError(const QString &)));
    FFeatures.append(AFeature);
    emit featureAdded(this,AFeature);
  }
}

void XmppStream::removeFeature(IStreamFeature *AFeature)
{
  if (FFeatures.contains(AFeature))
  {
    AFeature->instance()->disconnect(this);
    FFeatures.removeAt(FFeatures.indexOf(AFeature));
    emit featureRemoved(this,AFeature);

    IStreamFeaturePlugin *plugin = FXmppStreams->featurePlugin(AFeature->featureNS());
    if (plugin)
      plugin->destroyStreamFeature(AFeature);
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
  root.setAttribute("to", FJid.domain());
  root.setAttribute("version",xmppVersion());
  if (!FDefLang.isEmpty())
    root.setAttribute("xml:lang",FDefLang);
  QByteArray data = QString("<?xml version=\"1.0\"?>").toUtf8()+doc.toByteArray().trimmed();
  data.remove(data.size()-2,1);

  FStreamState = SS_INITIALIZE;
  if (!hookFeatureData(&data,IStreamFeature::DirectionOut))
  {
    showInConsole(doc.documentElement(),IStreamFeature::DirectionOut);
    FConnection->write(data);
  }
}

void XmppStream::abortStream(const QString &AError)
{
  if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
  {
    FStreamState = SS_ERROR;
    FLastError = AError;
    emit error(this,AError);
    FConnection->disconnect();
  }
}

void XmppStream::processFeatures()
{
  bool completed = true;
  QDomElement elem = FFeaturesElement.firstChildElement();
  while (!elem.isNull())
  {
    QDomElement nextElem = elem.nextSiblingElement();
    FFeaturesElement.removeChild(elem);
    if (startFeature(elem.namespaceURI(),elem))
    {
      completed = false;
      break;
    }
    else if (!elem.firstChildElement("required").isNull())
    {
      completed = false;
      abortStream(tr("Required stream feature '%1' not available").arg(elem.tagName()));
      break;
    }
    elem = nextElem;
  }
  if (completed)
  {
    FOpen = true;
    FStreamState = SS_ONLINE;
    emit opened(this);
  }
}

IStreamFeature *XmppStream::getFeature(const QString &AFeatureNS)
{
  foreach(IStreamFeature *feature, FFeatures)
    if (feature->featureNS() == AFeatureNS)
      return feature;

  IStreamFeaturePlugin *plugin = FXmppStreams->featurePlugin(AFeatureNS);
  if (plugin)
    return plugin->getStreamFeature(AFeatureNS,this);

  return NULL;
}

void XmppStream::sortFeature(IStreamFeature *AFeature)
{
  if (FFeatures.count() > 1)
  {
    if (AFeature)
    {
      if (!AFeature->needHook(IStreamFeature::DirectionIn))
      {
        FFeatures.replace(FFeatures.count()-1,AFeature);
        return;
      }
      int index = FFeatures.indexOf(AFeature);
      int i=0;
      while (i<FFeatures.count() && i != index && FFeatures.at(i)->needHook(IStreamFeature::DirectionIn))
        i++;
      if (i<FFeatures.count() && i != index)
        FFeatures.move(index,i);
    }
    else
    {
      int index=0;
      for (int i=0; i<FFeatures.count(); i++)
        if (FFeatures.at(index)->needHook(IStreamFeature::DirectionIn))
          index++;
        else
          FFeatures.move(index,FFeatures.count()-1);
    }
  }
}

bool XmppStream::startFeature(const QString &AFeatureNS, const QDomElement &AFeatureElem)
{
  IStreamFeature *feature = getFeature(AFeatureNS);
  if (feature)
  {
    addFeature(feature);
    if (feature->start(AFeatureElem))
      return true;
    else
      removeFeature(feature);
  }
  return false;
}

bool XmppStream::hookFeatureData(QByteArray *AData, IStreamFeature::Direction ADirection)
{
  if (ADirection == IStreamFeature::DirectionOut)
  {
    int i = FFeatures.count();
    while (i>0)
    {
      IStreamFeature *feature = FFeatures.at(--i);
      if (feature->needHook(ADirection))
        if (feature->hookData(AData,ADirection))
          return true;
    }
  }
  else
  {
    foreach(IStreamFeature *feature, FFeatures)
    {
      if (feature->needHook(ADirection))
        if (feature->hookData(AData,ADirection))
          return true;
    }
  }
  return false;
}

bool XmppStream::hookFeatureElement(QDomElement *AElem, IStreamFeature::Direction ADirection)
{
  if (ADirection == IStreamFeature::DirectionOut)
  {
    int i = FFeatures.count();
    while (i>0)
    {
      IStreamFeature *feature = FFeatures.at(--i);
      if (feature->needHook(ADirection))
        if (feature->hookElement(AElem,ADirection))
          return true;
    }
  }
  else
  {
    foreach(IStreamFeature *feature, FFeatures)
    {
      if (feature->needHook(ADirection))
        if (feature->hookElement(AElem,ADirection))
          return true;
    }
  }
  return false;
}

qint64 XmppStream::sendData(const QByteArray &AData)
{
  QByteArray data = AData;
  if (!hookFeatureData(&data,IStreamFeature::DirectionOut))
  {
    FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);
    return FConnection->write(data);
  }
  return 0;
}

QByteArray XmppStream::receiveData(qint64 ABytes)
{
  FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);
  return FConnection->read(ABytes);
}

void XmppStream::showInConsole(const QDomElement &AElem, IStreamFeature::Direction ADirection)
{
  emit consoleElement(this,AElem,ADirection==IStreamFeature::DirectionOut);
}

void XmppStream::onConnectionConnected()
{
  sortFeature();
  startStream();
}

void XmppStream::onConnectionReadyRead(qint64 ABytes)
{
  QByteArray data = receiveData(ABytes);
  if (!hookFeatureData(&data,IStreamFeature::DirectionIn))
    if (!data.isEmpty())
      FParser.parseData(data);
}

void XmppStream::onConnectionDisconnected()
{
  FOpen = false;
  emit closed(this);
  FStreamState = SS_OFFLINE;
  FKeepAliveTimer.stop();

  if (FOfflineJid.isValid())
  {
    setJid(FOfflineJid);
    FOfflineJid = Jid();
  }
}

void XmppStream::onConnectionError(const QString &AErrStr)
{
  abortStream(AErrStr);
}

void XmppStream::onParserOpened(const QDomElement &AElem)
{
  showInConsole(AElem,IStreamFeature::DirectionIn);

  if (AElem.namespaceURI() == NS_JABBER_STREAMS)
  {
    FStreamId = AElem.attribute("id");
    FStreamState = SS_FEATURES;
    if (VersionParser(AElem.attribute("version","0.0")) < VersionParser(1,0))
    {
      QDomDocument doc;
      doc.appendChild(doc.createElement("stream:features")).appendChild(doc.createElementNS(NS_FEATURE_IQAUTH,"auth"));
      FParser.parseData(doc.toByteArray(0));
    }
  }
  else
    abortStream(tr("Invalid stream namespace"));
}

void XmppStream::onParserElement(const QDomElement &AElem)
{
  showInConsole(AElem,IStreamFeature::DirectionIn);

  QDomElement elem(AElem);
  if (!hookFeatureElement(&elem,IStreamFeature::DirectionIn))
  {
    if(AElem.nodeName() == "stream:error")
    {
      ErrorHandler err(AElem,NS_XMPP_STREAMS);
      abortStream(err.message());
    }
    else if (FStreamState == SS_FEATURES && AElem.nodeName() == "stream:features")
    {
      FFeaturesElement = AElem;
      if (!startFeature(NS_FEATURE_REGISTER,QDomElement()))
        processFeatures();
    }
    else
    {
      emit element(this, elem);
    }
  }
}

void XmppStream::onParserError(const QString &AErrStr)
{
  abortStream(AErrStr);
}

void XmppStream::onParserClosed()
{
  FConnection->disconnect();
}

void XmppStream::onFeatureReady(bool ARestart)
{
  if (!ARestart)
    processFeatures();
  else
    startStream();
}

void XmppStream::onFeatureError(const QString &AError)
{
  FSessionPassword.clear();
  abortStream(AError);
}

void XmppStream::onKeepAliveTimeout()
{
  if (FStreamState!=SS_ONLINE)
    abortStream(tr("XMPP connection timed out"));
  else
    sendData(" ");
}

