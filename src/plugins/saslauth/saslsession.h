#ifndef SASLSESSION_H
#define SASLSESSION_H

#include <definations/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class SASLSession : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  SASLSession(IXmppStream *AXmppStream);
  ~SASLSession();
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_SESSION; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray &/*AData*/, Direction /*ADisrection*/) { return false; }
  virtual bool hookElement(QDomElement &AElem, Direction ADirection);
signals:
  virtual void ready(bool ARestart); 
  virtual void error(const QString &AError);
protected slots:
  void onStreamClosed();
private:
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
};

#endif // SASLSession_H
