#ifndef XMPPSTREAM_H
#define XMPPSTREAM_H

#include <QObject>
#include <QDomDocument>
#include "../../utils/versionparser.h"
#include "../../interfaces/ixmppstreams.h"
#include "streamparser.h"

class XmppStream : 
  public QObject,
  public IXmppStream
{
  Q_OBJECT

public:
  XmppStream(const Jid &AJid, QObject *parent = 0);
  virtual ~XmppStream();

  //IXmppStream
  virtual QObject *instance() { return this; }
  virtual bool isOpen() const { return FOpen; }
  virtual qint64 sendStanza(const Stanza &AStanza);
  virtual QString streamId() const { return FStreamId; }
  virtual QString lastError() const { return FLastError; };
  virtual void setJid(const Jid &AJid);
  virtual const Jid &jid() const { return FJid; }
  virtual void setHost(const QString &AHost) { FHost = AHost; }
  virtual QString host() const { return FHost; }
  virtual void setPort(const qint16 APort) { FPort = APort; }
  virtual qint16 port() const { return FPort; }
  virtual void setPassword(const QString &APassword) { FPassword = APassword; }
  virtual QString password() const { return FPassword; }
  virtual void setDefaultLang(const QString &ADefLang) { FDefLang = ADefLang; }
  virtual QString defaultLang() const { return FDefLang; }
  virtual void setXmppVersion(const QString &AXmppVersion) { FXmppVersion = AXmppVersion; }
  virtual QString xmppVersion() const { return FXmppVersion; }
  virtual IStreamConnection *connection();
  virtual bool setConnection(IStreamConnection *AConnection);
  virtual void addFeature(IStreamFeature *AStreamFeature);
  virtual void removeFeature(IStreamFeature *AStreamFeature);
  virtual QList<IStreamFeature *> &features() { return FFeatures; }
public slots:
  virtual void open();
  virtual void close();
signals:
  void opened(IXmppStream *);
  void element(IXmppStream *, const QDomElement &elem);
  void aboutToClose(IXmppStream *);
  void closed(IXmppStream *);
  void error(IXmppStream *, const QString &errStr);
protected slots:
  //IStreamConnection
  void onConnectionConnected();
  void onConnectionReadyRead(qint64 ABytes);
  void onConnectionDisconnected();
  void onConnectionError(const QString &AErrStr);
  //StreamParser
  void onParserOpen(const QDomElement &AElem);
  void onParserElement(const QDomElement &AElem);
  void onParserClosed();
  void onParserError(const QString &AErrStr);
  //IStreamFeature
  void onFeatureFinished(bool needRestart);
  void onFeatureError(const QString &AErrStr);
  void onFeatureDestroyed(QObject *AFeature);
protected:
  enum StreamState {
    SS_OFFLINE, 
    SS_CONNECTING, 
    SS_INITIALIZE, 
    SS_FEATURES, 
    SS_ONLINE 
  }; 
  virtual void startStream();
  virtual bool processFeatures(const QDomElement &AFeatures=QDomElement());
  virtual bool startFeature(const QString &AName, const QString &ANamespace);
  virtual bool startFeature(const QDomElement &AElem);
  virtual void sortFeature(IStreamFeature *AFeature=0);
  StreamParser FParser;
  QString FLastError;
  StreamState FStreamState; 
  QDomElement FActiveFeatures;
  bool FOpen;
private:
  Jid FJid;
  IStreamConnection *FConnection;
  QString FStreamId; 
  QString FHost;
  qint16 FPort;
  QString FPassword;
  QString FDefLang;
  QString FXmppVersion;
  QList<IStreamFeature *>	FFeatures; 
};

#endif // XMPPSTREAM_H
