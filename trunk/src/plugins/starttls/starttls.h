#ifndef STARTTLS_H
#define STARTTLS_H

#include <QObject>
#include <definations/namespaces.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <utils/stanza.h>

class StartTLS : 
  public QObject,
  public IXmppFeature,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppElementHadler);
public:
  StartTLS(IXmppStream *AXmppStream);
  ~StartTLS();
  virtual QObject *instance() { return this; }
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppFeature
  virtual QString featureNS() const { return NS_FEATURE_STARTTLS; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart);
  void error(const QString &AMessage);
  void featureDestroyed();
protected slots:
  void onConnectionEncrypted();
private: 
  IXmppStream *FXmppStream;
  IDefaultConnection *FConnection;
};

#endif // STARTTLS_H
