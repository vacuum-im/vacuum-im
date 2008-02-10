#ifndef REGISTERSTREAM_H
#define REGISTERSTREAM_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/iregistraton.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

class RegisterStream : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  RegisterStream(IStreamFeaturePlugin *ARegistration, IXmppStream *AXmppStream);
  ~RegisterStream();
  virtual QObject *instance() { return this; }
  //IStreamFeature
  virtual QString name() const { return "register"; }
  virtual QString nsURI() const { return NS_FEATURE_REGISTER; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray * /*AData*/, Direction /*ADirection*/) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void finished(bool ARestartStream); 
  virtual void error(const QString &AMessage);
  virtual void destroyed(IStreamFeature *AFeature);
private:
  IXmppStream *FXmppStream;
  IStreamFeaturePlugin *FFeaturePlugin;
private:
  bool FNeedHook;
  bool FRegisterFinished;
};

#endif // REGISTERSTREAM_H
