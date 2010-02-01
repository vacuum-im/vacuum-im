#ifndef REGISTERSTREAM_H
#define REGISTERSTREAM_H

#include <definations/namespaces.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iregistraton.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class RegisterStream :
  public QObject,
  public IXmppFeature,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppElementHadler);
public:
  RegisterStream(IXmppStream *AXmppStream);
  ~RegisterStream();
  virtual QObject *instance() { return this; }
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppFeature
  virtual QString featureNS() const { return NS_FEATURE_REGISTER; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart); 
  void error(const QString &AMessage);
  void featureDestroyed();
private:
  IXmppStream *FXmppStream;
};

#endif // REGISTERSTREAM_H
