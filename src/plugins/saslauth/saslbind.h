#ifndef SASLBIND_H
#define SASLBIND_H

#include <definations/namespaces.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class SASLBind : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  SASLBind(IXmppStream *AXmppStream);
  ~SASLBind();
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_BIND; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray &/*AData*/, Direction /*ADirection*/) { return false; }
  virtual bool hookElement(QDomElement &AElem, Direction ADirection);
signals:
  void ready(bool ARestart); 
  void error(const QString &AError);
protected slots:
  void onStreamClosed();
private:
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
};

#endif // SASLBIND_H
