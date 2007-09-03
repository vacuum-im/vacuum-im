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
  Q_OBJECT;
  Q_INTERFACES(IXmppStream);

public:
  XmppStream(const Jid &AJid, QObject *parent = 0);
  ~XmppStream();

  //IXmppStream
  virtual QObject *instance() { return this; }
  virtual bool isOpen() const { return FOpen; }
  virtual void open();
  virtual void close();
  virtual qint64 sendStanza(const Stanza &AStanza);
  virtual const QString &streamId() const { return FStreamId; }
  virtual const QString &lastError() const { return FLastError; };
  virtual void setJid(const Jid &AJid);
  virtual const Jid &jid() const { return FJid; }
  virtual const QString &password() const { return FPassword; }
  virtual void setPassword(const QString &APassword) { FPassword = APassword; }
  virtual const QString &defaultLang() const { return FDefLang; }
  virtual void setDefaultLang(const QString &ADefLang) { FDefLang = ADefLang; }
  virtual const QString &xmppVersion() const { return FXmppVersion; }
  virtual void setXmppVersion(const QString &AXmppVersion) { FXmppVersion = AXmppVersion; }
  virtual IConnection *connection() const;
  virtual void setConnection(IConnection *AConnection);
  virtual void addFeature(IStreamFeature *AStreamFeature);
  virtual void removeFeature(IStreamFeature *AStreamFeature);
  virtual QList<IStreamFeature *> features() const { return FFeatures; }
signals:
  virtual void opened(IXmppStream *AXmppStream);
  virtual void element(IXmppStream *AXmppStream, const QDomElement &elem);
  virtual void aboutToClose(IXmppStream *AXmppStream);
  virtual void closed(IXmppStream *AXmppStream);
  virtual void error(IXmppStream *AXmppStream, const QString &errStr);
  virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  virtual void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection);
  virtual void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection);
protected:
  enum StreamState {
    SS_OFFLINE, 
    SS_CONNECTING, 
    SS_INITIALIZE, 
    SS_FEATURES, 
    SS_ONLINE 
  }; 
  void startStream();
  bool processFeatures(const QDomElement &AFeatures=QDomElement());
  bool startFeature(const QString &AName, const QString &ANamespace);
  bool startFeature(const QDomElement &AElem);
  void sortFeature(IStreamFeature *AFeature=0);
  bool hookFeatureData(QByteArray *AData, IStreamFeature::Direction ADirection);
  bool hookFeatureElement(QDomElement *AElem, IStreamFeature::Direction ADirection);
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
private:
  IConnection *FConnection;
  QList<IStreamFeature *>	FFeatures; 
private:
  bool FOpen;
  Jid FJid;
  Jid FOfflineJid;
  QString FStreamId; 
  QString FPassword;
  QString FDefLang;
  QString FXmppVersion;
  QString FLastError;
  StreamState FStreamState; 
  StreamParser FParser;
  QDomElement FActiveFeatures;
};

#endif // XMPPSTREAM_H
