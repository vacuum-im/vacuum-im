#ifndef XMPPSTREAM_H
#define XMPPSTREAM_H

#include <QTimer>
#include <QDomDocument>
#include "../../definations/namespaces.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/errorhandler.h"
#include "../../utils/versionparser.h"
#include "streamparser.h"

class XmppStream : 
  public QObject,
  public IXmppStream
{
  Q_OBJECT;
  Q_INTERFACES(IXmppStream);
public:
  XmppStream(IXmppStreams *AXmppStreams, const Jid &AJid);
  ~XmppStream();
  virtual QObject *instance() { return this; }
  //IXmppStream
  virtual bool isOpen() const { return FOpen; }
  virtual void open();
  virtual void close();
  virtual qint64 sendStanza(const Stanza &AStanza);
  virtual const QString &streamId() const { return FStreamId; }
  virtual const QString &lastError() const { return FLastError; };
  virtual const Jid &jid() const { return FJid; }
  virtual void setJid(const Jid &AJid);
  virtual const QString &password() const;
  virtual void setPassword(const QString &APassword) { FPassword = APassword; }
  virtual const QString &defaultLang() const { return FDefLang; }
  virtual void setDefaultLang(const QString &ADefLang) { FDefLang = ADefLang; }
  virtual const QString &xmppVersion() const { return FXmppVersion; }
  virtual void setXmppVersion(const QString &AXmppVersion) { FXmppVersion = AXmppVersion; }
  virtual IConnection *connection() const;
  virtual void setConnection(IConnection *AConnection);
  virtual void addFeature(IStreamFeature *AFeature);
  virtual QList<IStreamFeature *> features() const { return FFeatures; }
  virtual void removeFeature(IStreamFeature *AFeature);
signals:
  virtual void opened(IXmppStream *AXmppStream);
  virtual void element(IXmppStream *AXmppStream, const QDomElement &elem);
  virtual void aboutToClose(IXmppStream *AXmppStream);
  virtual void error(IXmppStream *AXmppStream, const QString &errStr);
  virtual void closed(IXmppStream *AXmppStream);
  virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  virtual void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection);
  virtual void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection);
  virtual void featureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature);
  virtual void featureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature);
  virtual void destroyed(IXmppStream *AXmppStream);
protected:
  void startStream();
  void abortStream(const QString &AError);
  void processFeatures();
  IStreamFeature *getFeature(const QString &AFeatureNS);
  void sortFeature(IStreamFeature *AFeature = NULL);
  bool startFeature(const QString &AFeatureNS, const QDomElement &AFeatureElem);
  bool hookFeatureData(QByteArray *AData, IStreamFeature::Direction ADirection);
  bool hookFeatureElement(QDomElement *AElem, IStreamFeature::Direction ADirection);
  qint64 sendData(const QByteArray &AData);
  QByteArray receiveData(qint64 ABytes);
  void showInConsole(const QDomElement &AElem, IStreamFeature::Direction ADirection);
protected slots:
  //IStreamConnection
  void onConnectionConnected();
  void onConnectionReadyRead(qint64 ABytes);
  void onConnectionDisconnected();
  void onConnectionError(const QString &AErrStr);
  //StreamParser
  void onParserOpened(const QDomElement &AElem);
  void onParserElement(const QDomElement &AElem);
  void onParserClosed();
  void onParserError(const QString &AErrStr);
  //IStreamFeature
  void onFeatureReady(bool ARestart);
  void onFeatureError(const QString &AError);
  //KeepAlive
  void onKeepAliveTimeout();
private:
  IXmppStreams *FXmppStreams;
  IConnection *FConnection;
private:
  QDomElement FFeaturesElement;
  IStreamFeature *FActiveFeature;
  QList<IStreamFeature *>	FFeatures; 
private:
  enum StreamState {
    SS_OFFLINE, 
    SS_CONNECTING, 
    SS_INITIALIZE, 
    SS_FEATURES, 
    SS_ONLINE,
    SS_ERROR
  } FStreamState; 
  bool FOpen;
  Jid FJid;
  Jid FOfflineJid;
  QString FStreamId; 
  QString FPassword;
  QString FSessionPassword;
  QString FDefLang;
  QString FXmppVersion;
  QString FLastError;
  StreamParser FParser;
  QTimer FKeepAliveTimer;
};

#endif // XMPPSTREAM_H
