#ifndef SASLSESSION_H
#define SASLSESSION_H

#include <definations/namespaces.h>
#include <definations/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class SASLSession : 
  public QObject,
  public IXmppFeature,
  public IXmppStanzaHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
  SASLSession(IXmppStream *AXmppStream);
  ~SASLSession();
  //IXmppStanzaHadler
  virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
  virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
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
