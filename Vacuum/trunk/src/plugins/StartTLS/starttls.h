#ifndef STARTTLS_H
#define STARTTLS_H

#include <QObject>
#include "../../definations/namespaces.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/idefaultconnection.h"
#include "../../utils/stanza.h"

class StartTLS : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  StartTLS(IXmppStream *AXmppStream);
  ~StartTLS();
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_STARTTLS; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *, Direction) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void ready(bool ARestart); 
  virtual void error(const QString &AMessage);
protected slots:
  void onStreamClosed(IXmppStream *AXmppStream);
private: 
  IXmppStream *FXmppStream;
  IDefaultConnection *FConnection;
private:
  bool FNeedHook;
};

#endif // STARTTLS_H
