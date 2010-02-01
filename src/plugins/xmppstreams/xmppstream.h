#ifndef XMPPSTREAM_H
#define XMPPSTREAM_H

#include <QTimer>
#include <QMultiMap>
#include <QDomDocument>
#include <definations/namespaces.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <utils/errorhandler.h>
#include <utils/versionparser.h>
#include "streamparser.h"

enum StreamState 
{
  SS_OFFLINE, 
  SS_CONNECTING, 
  SS_INITIALIZE, 
  SS_FEATURES, 
  SS_ONLINE,
  SS_ERROR
}; 

class XmppStream : 
  public QObject,
  public IXmppStream,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppStream IXmppElementHadler);
public:
  XmppStream(IXmppStreams *AXmppStreams, const Jid &AStreamJid);
  ~XmppStream();
  virtual QObject *instance() { return this; }
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppStream
  virtual bool isOpen() const;
  virtual bool open();
  virtual void close();
  virtual void abort(const QString &AError);
  virtual QString streamId() const;
  virtual QString errorString() const;;
  virtual Jid streamJid() const;
  virtual void setStreamJid(const Jid &AJid);
  virtual QString password() const;
  virtual void setPassword(const QString &APassword);
  virtual QString defaultLang() const;
  virtual void setDefaultLang(const QString &ADefLang);
  virtual IConnection *connection() const;
  virtual void setConnection(IConnection *AConnection);
  virtual qint64 sendStanza(const Stanza &AStanza);
  virtual void insertXmppDataHandler(IXmppDataHandler *AHandler, int AOrder);
  virtual void removeXmppDataHandler(IXmppDataHandler *AHandler, int AOrder);
  virtual void insertXmppElementHandler(IXmppElementHadler *AHandler, int AOrder);
  virtual void removeXmppElementHandler(IXmppElementHadler *AHandler, int AOrder);
signals:
  void opened();
  void aboutToClose();
  void closed();
  void error(const QString &AError);
  void jidAboutToBeChanged(const Jid &AAfter);
  void jidChanged(const Jid &ABefour);
  void connectionChanged(IConnection *AConnection);
  void dataHandlerInserted(IXmppDataHandler *AHandler, int AOrder);
  void dataHandlerRemoved(IXmppDataHandler *AHandler, int AOrder);
  void elementHandlerInserted(IXmppElementHadler *AHandler, int AOrder);
  void elementHandlerRemoved(IXmppElementHadler *AHandler, int AOrder);
  void streamDestroyed();
protected:
  void startStream();
  void processFeatures();
  bool startFeature(const QString &AFeatureNS, const QDomElement &AFeatureElem);
  bool processDataHandlers(QByteArray &AData, bool ADataOut);
  bool processElementHandlers(QDomElement &AElem, bool AElementOut);
  qint64 sendData(const QByteArray &AData);
  QByteArray receiveData(qint64 ABytes);
protected slots:
  //IStreamConnection
  void onConnectionConnected();
  void onConnectionReadyRead(qint64 ABytes);
  void onConnectionError(const QString &AError);
  void onConnectionDisconnected();
  //StreamParser
  void onParserOpened(QDomElement AElem);
  void onParserElement(QDomElement AElem);
  void onParserError(const QString &AError);
  void onParserClosed();
  //IXmppFeature
  void onFeatureFinished(bool ARestart);
  void onFeatureError(const QString &AError);
  void onFeatureDestroyed();
  //KeepAlive
  void onKeepAliveTimeout();
private:
  IXmppStreams *FXmppStreams;
  IConnection *FConnection;
private:
  QDomElement FServerFeatures;
  QList<QString>	FAvailFeatures; 
  QList<IXmppFeature *>	FActiveFeatures; 
private:
  QMultiMap<int, IXmppDataHandler *> FDataHandlers;
  QMultiMap<int, IXmppElementHadler *> FElementHandlers;
private:
  bool FOpen;
  Jid FStreamJid;
  Jid FOfflineJid;
  QString FStreamId; 
  QString FPassword;
  QString FSessionPassword;
  QString FDefLang;
  QString FErrorString;
  StreamParser FParser;
  QTimer FKeepAliveTimer;
  StreamState FStreamState;
};

#endif // XMPPSTREAM_H
