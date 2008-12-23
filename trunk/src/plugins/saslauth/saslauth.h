#ifndef SASLAUTH_H
#define SASLAUTH_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

class SASLAuth :
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  SASLAuth(IXmppStream *AXmppStream);
  ~SASLAuth();
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_SASL; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray * /*AData*/, Direction /*ADirection*/) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void ready(bool ARestart); 
  virtual void error(const QString &AError);
protected slots:
  void onStreamClosed(IXmppStream *AXmppStream);
private: 
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
  bool FAuthorized;
  int FChallengeStep;
  QString FMechanism;
};

#endif // SASLAUTH_H
