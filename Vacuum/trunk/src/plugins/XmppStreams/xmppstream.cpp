#include "xmppstream.h"
#include <QtDebug>
#include "utils/errorhandler.h"
#include "streamconnection.h"

XmppStream::XmppStream(const Jid &AJid, QObject *parent)
  : QObject(parent), 
    FParser(this)
{
  FJid = AJid;
  FConnection = 0;
  FOpen = false;
  FStreamState = SS_OFFLINE;
  FPort = 0;
  FHost = AJid.domane();
  FPort = 5222;

  connect(&FParser,SIGNAL(open(const QDomElement &)), SLOT(onParserOpen(const QDomElement &)));
  connect(&FParser,SIGNAL(element(const QDomElement &)), SLOT(onParserElement(const QDomElement &)));
  connect(&FParser,SIGNAL(error(const QString &)), SLOT(onParserError(const QString &)));
  connect(&FParser,SIGNAL(closed()), SLOT(onParserClosed()));
}

XmppStream::~XmppStream()
{
  qDebug() << "~XmppStream " << FJid.full();
  close();
}

void XmppStream::open()
{
  if (FStreamState == SS_OFFLINE)
  {
    FStreamState = SS_CONNECTING;
    connection()->connectToHost(FHost,FPort);
  }
}

void XmppStream::close()
{	
  if (FStreamState == SS_ONLINE)
    emit aboutToClose(this);

  if (FConnection) if (FConnection->isOpen())
  {
    if (FConnection->isValid())
      FConnection->write("</stream:stream>"); 
    FConnection->close();
  }

  FStreamState = SS_OFFLINE;
}

void XmppStream::setJid(const Jid &AJid) 
{
  if (FStreamState == SS_OFFLINE || FStreamState == SS_FEATURES) 
    FJid = AJid;
}

qint64 XmppStream::sendStanza(const Stanza &stanza)
{
  if (FStreamState != SS_OFFLINE)
  {
    qDebug() << "\nStanza out" << FJid.full()<< ":\n" << stanza.toString(); 
    return FConnection->write(stanza.toByteArray());
  }
  return false;
}

IStreamConnection *XmppStream::connection()
{
  if (!FConnection)
    setConnection(new StreamConnection(this));

  return FConnection;
}

bool XmppStream::setConnection(IStreamConnection *AConnection)
{
  if (FConnection == AConnection)
    return true;

  if (FConnection)
  {
    FConnection->instance()->~QObject();
    FConnection = 0;
  }

  if (AConnection)
  {
    FConnection = AConnection;
    connect(FConnection->instance(),SIGNAL(connected()),SLOT(onConnectionConnected()));
    connect(FConnection->instance(),SIGNAL(readyRead(qint64)),SLOT(onConnectionReadyRead(qint64)));
    connect(FConnection->instance(),SIGNAL(disconnected()),SLOT(onConnectionDisconnected()));
    connect(FConnection->instance(),SIGNAL(error(const QString &)),SLOT(onConnectionError(const QString &)));
  }

  return true;
}

void XmppStream::addFeature(IStreamFeature *AStreamFeature) 
{
  if (!FFeatures.contains(AStreamFeature))
  {
    QObject *obj = AStreamFeature->instance();
    connect(obj,SIGNAL(finished(bool)),SLOT(onFeatureFinished(bool)));
    connect(obj,SIGNAL(error(const QString &)),SLOT(onFeatureError(const QString &)));
    connect(obj,SIGNAL(destroyed(QObject *)),SLOT(onFeatureDestroyed(QObject *)));
    FFeatures.append(AStreamFeature);
  }
}

void XmppStream::removeFeature(IStreamFeature *AStreamFeature)
{
  FFeatures.removeAt(FFeatures.indexOf(AStreamFeature));
}

void XmppStream::startStream()
{
  FStreamState = SS_INITIALIZE;
  FParser.clear();
  QDomDocument doc;
  QDomElement root = doc.createElementNS(NS_JABBER_STREAMS,"stream:stream"); 
  doc.appendChild(root); 
  root.setAttribute("xmlns",NS_JABBER_CLIENT);
  root.setAttribute("to", FJid.domane());
  root.setAttribute("version",xmppVersion()); 
  if (!FDefLang.isEmpty())
    root.setAttribute("xml:lang",FDefLang); 
  QString xml = doc.toString().trimmed();
  xml.remove(xml.length()-2,1);
  FConnection->write(("<?xml version=\"1.0\"?>"+xml).toUtf8());
}

bool XmppStream::processFeatures(const QDomElement &AFeatures) 
{
  if (!AFeatures.isNull())
    FActiveFeatures = AFeatures;

  int i = 0;
  QDomElement elem;
  while (i<FActiveFeatures.childNodes().count())
  {
    elem = FActiveFeatures.childNodes().item(i++).toElement();
    FActiveFeatures.removeChild(elem); 
    if (startFeature(elem))
      return true;
  }
  return false;
}

bool XmppStream::startFeature(const QString &AName, const QString &ANamespace) 
{
  foreach(IStreamFeature *feature, FFeatures)
  {
    if (feature->name() == AName && feature->nsURI() == ANamespace)
      if (feature->start(QDomElement()))
      {
        sortFeature(feature);
        return true;
      } 
      else 
        return false;
  }
  return false;
}

bool XmppStream::startFeature(const QDomElement &AElem)
{
  QString name = AElem.nodeName();
  QString nsURI = AElem.namespaceURI();
  foreach(IStreamFeature *feature, FFeatures)
  {
    if (feature->name() == name && feature->nsURI() == nsURI)
      if (feature->start(AElem))
      {
        sortFeature(feature);
        return true;
      } 
      else 
        return false;
  }
  return false;
}

void XmppStream::sortFeature(IStreamFeature *AFeature)
{
  if (!(FFeatures.count()>1)) return;
  if (AFeature)
  {
    if (!AFeature->needHook()) 
    {
      FFeatures.replace(FFeatures.count()-1,AFeature); 
      return; 
    }
    int index = FFeatures.indexOf(AFeature);
    int i=0;
    while (i<FFeatures.count() && i != index && FFeatures[i]->needHook())
      i++;
    if (i<FFeatures.count() && i != index)
      FFeatures.move(index,i);
  }
  else
  {
    int index=0;
    for (int i=0; i<FFeatures.count(); i++)
      if (FFeatures[index]->needHook())
        index++;
      else
        FFeatures.move(index,FFeatures.count()-1);  
  }
}

void XmppStream::onConnectionConnected() 
{
  sortFeature();
  startStream();
}

void XmppStream::onConnectionReadyRead(qint64 ABytes)
{
  QByteArray data = FConnection->read(ABytes);
  int i = 0;
  IStreamFeature *feature;
  while (i<FFeatures.count()) 
  {
    feature = FFeatures[i++];
    if (feature->needHook())
      if (feature->hookData(&data))
        return; 
  }
  if (!data.isEmpty())
    FParser.parceData(QString::fromUtf8(data.data()));
}

void XmppStream::onConnectionDisconnected()
{
  FOpen = false;
  emit closed(this);
  FStreamState = SS_OFFLINE;
}

void XmppStream::onConnectionError(const QString &AErrStr)
{
  qDebug() << "CONNECTION" << FJid.full() << "ERROR:" << AErrStr;
  FLastError = AErrStr;
  emit error(this, AErrStr);
}

void XmppStream::onParserOpen(const QDomElement &AElem)
{
  if (AElem.namespaceURI() == NS_JABBER_STREAMS)
  {
    FStreamId = AElem.attribute("id");
    FStreamState = SS_FEATURES;
    if (VersionParser(AElem.attribute("version","0.0")) < VersionParser(1,0))
    {
      qDebug() << "Features are not supported by server or not requested";
      if (!startFeature("auth","http://jabber.org/features/iq-auth"))
        close();
    }
  } else close();
}

void XmppStream::onParserElement(const QDomElement &AElem)
{
  qDebug() << "\nStanza in" << FJid.full() << ":\n" << AElem.ownerDocument().toString();   
  
  IStreamFeature *feature; 
  QDomElement elem(AElem);
  int i = 0;
  while (i<FFeatures.count()) 
  {
    feature = FFeatures[i++];
    if (feature->needHook())
      if (feature->hookElement(&elem)) 
        return;
  }

  if(AElem.tagName() == "stream:error")
  {
    ErrorHandler err(NS_XMPP_STREAMS,AElem);
    err.setContext(tr("During stream negotiation there was an error:")); 
    emit error(this, err.message());

    FConnection->close();
  } 
  else if (FStreamState == SS_FEATURES && AElem.tagName() == "stream:features")
  {
    if (!processFeatures(AElem))
    {
      FStreamState = SS_ONLINE;
      FOpen = false;
      emit opened(this);
    }
  } 
  else
  {
    emit element(this, elem);
  }
}

void XmppStream::onParserError(const QString &AErrStr)
{
  FLastError = AErrStr;
  emit error(this, AErrStr);

  FConnection->close();
}

void XmppStream::onParserClosed() 
{
  FConnection->close(); 
}

void XmppStream::onFeatureFinished(bool needRestart) 
{
  if (!needRestart)
  {
    if (!processFeatures())
    {
      FStreamState = SS_ONLINE;
      FOpen = true;
      emit opened(this);
    }
  }
  else
    startStream();
}

void XmppStream::onFeatureError(const QString &AErrStr)
{
  FLastError = AErrStr;
  emit error(this, AErrStr);

  FConnection->close();
}

void XmppStream::onFeatureDestroyed(QObject *AFeature)
{
  removeFeature((IStreamFeature *)AFeature);
}
