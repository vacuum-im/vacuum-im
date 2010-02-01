#ifndef SASLSESSION_H
#define SASLSESSION_H

#include <definations/namespaces.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class SASLSession : 
  public QObject,
  public IXmppFeature,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppElementHadler);
public:
  SASLSession(IXmppStream *AXmppStream);
  ~SASLSession();
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppFeature
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_SESSION; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart); 
  void error(const QString &AError);
  void featureDestroyed();
private:
  IXmppStream *FXmppStream;
};

#endif // SASLSESSION_H
