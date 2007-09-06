#include <QtDebug>
#include "xmppstream.h"

#include <QInputDialog>
#include "../../definations/namespaces.h"
#include "../../utils/errorhandler.h"

XmppStream::XmppStream(const Jid &AJid, QObject *parent)
  : QObject(parent), 
    FParser(this)
{
  FOpen = false;
  FJid = AJid;
  FXmppVersion = "1.0";
  FConnection = NULL;
  FStreamState = SS_OFFLINE;

  connect(&FParser,SIGNAL(open(const QDomElement &)), SLOT(onParserOpen(const QDomElement &)));
  connect(&FParser,SIGNAL(element(const QDomElement &)), SLOT(onParserElement(const QDomElement &)));
  connect(&FParser,SIGNAL(error(const QString &)), SLOT(onParserError(const QString &)));
  connect(&FParser,SIGNAL(closed()), SLOT(onParserClosed()));
}

XmppStream::~XmppStream()
{
  close();
}

void XmppStream::open()
{
  if (FConnection && FStreamState == SS_OFFLINE)
  {
    if (FPassword.isEmpty())
    {
      FSessionPassword = QInputDialog::getText(NULL,tr("Password request"),tr("Enter password for <b>%1</b>").arg(FJid.bare()),
        QLineEdit::Password,"",NULL,Qt::Dialog);
    }

    FStreamState = SS_CONNECTING;
    FConnection->connectToHost();
  }
  else if (!FConnection)
   emit error(this, tr("No connection specified"));
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
  if (FJid != AJid && (FStreamState == SS_OFFLINE || (FStreamState == SS_FEATURES && FJid.equals(AJid,false)))) 
  {
    if (FStreamState == SS_FEATURES && !FOfflineJid.isValid())
      FOfflineJid = FJid;

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
  if (FStreamState != SS_OFFLINE)
  {
    Stanza stanza(AStanza);
    if (!hookFeatureElement(&stanza.element(),IStreamFeature::DirectionOut))
    {
      qDebug() << "\nStanza out" << FJid.full()<< ":\n" << stanza.toString(); 
      QByteArray data = stanza.toByteArray();
      if (!hookFeatureData(&data,IStreamFeature::DirectionOut))
        return FConnection->write(data);
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
      disconnect(FConnection->instance(),SIGNAL(connected()),this,SLOT(onConnectionConnected()));
      disconnect(FConnection->instance(),SIGNAL(readyRead(qint64)),this,SLOT(onConnectionReadyRead(qint64)));
      disconnect(FConnection->instance(),SIGNAL(disconnected()),this,SLOT(onConnectionDisconnected()));
      disconnect(FConnection->instance(),SIGNAL(error(const QString &)),this,SLOT(onConnectionError(const QString &)));
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

void XmppStream::addFeature(IStreamFeature *AStreamFeature) 
{
  if (AStreamFeature && !FFeatures.contains(AStreamFeature))
  {
    connect(AStreamFeature->instance(),SIGNAL(finished(bool)),SLOT(onFeatureFinished(bool)));
    connect(AStreamFeature->instance(),SIGNAL(error(const QString &)),SLOT(onFeatureError(const QString &)));
    connect(AStreamFeature->instance(),SIGNAL(destroyed(QObject *)),SLOT(onFeatureDestroyed(QObject *)));
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
  
  QByteArray data = QString("<?xml version=\"1.0\"?>").toUtf8()+doc.toByteArray().trimmed();
  data.remove(data.size()-2,1);
  if (!hookFeatureData(&data,IStreamFeature::DirectionOut))
    FConnection->write(data);
}

bool XmppStream::processFeatures(const QDomElement &AFeatures) 
{
  if (!AFeatures.isNull())
    FActiveFeatures = AFeatures;

  QDomElement elem;
  while (FActiveFeatures.childNodes().count()>0)
  {
    elem = FActiveFeatures.childNodes().item(0).toElement();
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
  if (FFeatures.count() < 2) return;

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

void XmppStream::onConnectionConnected() 
{
  sortFeature();
  startStream();
}

void XmppStream::onConnectionReadyRead(qint64 ABytes)
{
  QByteArray data = FConnection->read(ABytes);
  if (!hookFeatureData(&data,IStreamFeature::DirectionIn))
    if (!data.isEmpty())
      FParser.parceData(QString::fromUtf8(data.data()));
}

void XmppStream::onConnectionDisconnected()
{
  FOpen = false;
  emit closed(this);
  FStreamState = SS_OFFLINE;

  if (FOfflineJid.isValid())
  {
    setJid(FOfflineJid);
    FOfflineJid = Jid();
  }
}

void XmppStream::onConnectionError(const QString &AErrStr)
{
  qDebug() << "CONNECTION" << FJid.full() << "ERROR:" << AErrStr;
  FLastError = AErrStr;
  FStreamState = SS_OFFLINE;
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
      if (!startFeature("auth","http://jabber.org/features/iq-auth"))
        close();
    }
  } else close();
}

void XmppStream::onParserElement(const QDomElement &AElem)
{
  qDebug() << "\nStanza in" << FJid.full() << ":\n" << AElem.ownerDocument().toString();   
  
  QDomElement elem(AElem);
  if (!hookFeatureElement(&elem,IStreamFeature::DirectionIn))
  {
    if(AElem.tagName() == "stream:error")
    {
      ErrorHandler err(NS_XMPP_STREAMS,AElem);
      err.setContext(tr("During stream negotiation there was an error:")); 
      emit error(this, err.message());

      FConnection->disconnect();
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
}

void XmppStream::onParserError(const QString &AErrStr)
{
  FLastError = AErrStr;
  emit error(this, AErrStr);

  FConnection->disconnect();
}

void XmppStream::onParserClosed() 
{
  FConnection->disconnect(); 
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

  FConnection->disconnect();
}

void XmppStream::onFeatureDestroyed(QObject *AFeature)
{
  IStreamFeature *feature = dynamic_cast<IStreamFeature *>(AFeature);
  removeFeature(feature);
}

