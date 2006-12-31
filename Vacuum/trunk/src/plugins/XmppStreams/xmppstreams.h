#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include "interfaces/ipluginmanager.h"
#include "interfaces/ixmppstreams.h"
#include "interfaces/isettings.h"

#define XMPPSTREAMS_UUID "{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"

class XmppStreams :
  public QObject,
  public IPlugin,
  public IXmppStreams
{
  Q_OBJECT
  Q_INTERFACES(IPlugin IXmppStreams)

public:
  XmppStreams();
  ~XmppStreams();
  virtual QObject *instance() {return this;}

  //IPlugin
  virtual QUuid getPluginUuid() const { return XMPPSTREAMS_UUID;}
  virtual void getPluginInfo(PluginInfo *);
  virtual bool initPlugin(IPluginManager *);
  virtual bool startPlugin();

  //IXmppStreams
  virtual IXmppStream *newStream(const Jid &AJid);
  virtual bool addStream(IXmppStream *);
  virtual IXmppStream *getStream(const Jid &AJid) const;
  virtual const QList<IXmppStream *> &getStreams() const { return FStreams; }
  virtual void removeStream(IXmppStream *);
signals:
  virtual void added(IXmppStream *);
  virtual void opened(IXmppStream *);
  virtual void element(IXmppStream *, const QDomElement &elem);
  virtual void aboutToClose(IXmppStream *);
  virtual void closed(IXmppStream *);
  virtual void error(IXmppStream *, const QString &errStr);
  virtual void removed(IXmppStream *);
protected slots:
  //IXmppStream
  void onStreamOpened(IXmppStream *);
  void onStreamElement(IXmppStream *, const QDomElement &elem);
  void onStreamAboutToClose(IXmppStream *);
  void onStreamClosed(IXmppStream *);
  void onStreamError(IXmppStream *, const QString &AErrStr);
  //IConfigurator
  void onConfigOpened();
  void onConfigClosed();
private: //interfaces
  IPluginManager *FPluginManager;
  ISettings *FSettings; 
private:
  QList<IXmppStream *> FStreams;
  QList<IXmppStream *> FAddedStreams;
};

#endif // XMPPSTREAMS_H
