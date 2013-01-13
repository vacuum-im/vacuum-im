#ifndef SASLBIND_H
#define SASLBIND_H

#include <definations/namespaces.h>
#include <definations/xmppstanzahandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class SASLBind : 
  public QObject,
  public IXmppFeature,
  public IXmppStanzaHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
  SASLBind(IXmppStream *AXmppStream);
  ~SASLBind();
  virtual QObject *instance() { return this; }
  //IXmppStanzaHadler
  virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
  virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
  //IXmppFeature
  virtual QString featureNS() const { return NS_FEATURE_BIND; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart); 
  void error(const QString &AError);
  void featureDestroyed();
private:
  IXmppStream *FXmppStream;
};

#endif // SASLBIND_H
