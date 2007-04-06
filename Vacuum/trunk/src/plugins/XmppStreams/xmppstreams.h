#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/isettings.h"

#define XMPPSTREAMS_UUID "{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"

class XmppStreams :
  public QObject,
  public IPlugin,
  public IXmppStreams
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IXmppStreams);

public:
  XmppStreams();
  ~XmppStreams();
  virtual QObject *instance() {return this;}

  //IPlugin
  virtual QUuid pluginUuid() const { return XMPPSTREAMS_UUID;}
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IXmppStreams
  virtual IXmppStream *newStream(const Jid &AJid);
  virtual void addStream(IXmppStream *AStream);
  virtual bool isActive(IXmppStream *AStream) const { return FActiveStreams.contains(AStream); }
  virtual IXmppStream *getStream(const Jid &AJid) const;
  virtual const QList<IXmppStream *> &getStreams() const { return FStreams; }
  virtual void removeStream(IXmppStream *AStream);
  virtual void destroyStream(const Jid &AJid);
signals:
  virtual void added(IXmppStream *);
  virtual void opened(IXmppStream *);
  virtual void element(IXmppStream *, const QDomElement &elem);
  virtual void aboutToClose(IXmppStream *);
  virtual void closed(IXmppStream *);
  virtual void error(IXmppStream *, const QString &errStr);
  virtual void jidAboutToBeChanged(IXmppStream *, const Jid &AAfter);
  virtual void jidChanged(IXmppStream *, const Jid &ABefour);
  virtual void removed(IXmppStream *);
protected slots:
  void onStreamOpened(IXmppStream *);
  void onStreamElement(IXmppStream *, const QDomElement &elem);
  void onStreamAboutToClose(IXmppStream *);
  void onStreamClosed(IXmppStream *);
  void onStreamError(IXmppStream *, const QString &AErrStr);
  void onStreamJidAboutToBeChanged(IXmppStream *AStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour);
private:
  QList<IXmppStream *> FStreams;
  QList<IXmppStream *> FActiveStreams;
};

#endif // XMPPSTREAMS_H
