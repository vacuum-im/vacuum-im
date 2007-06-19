#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/isettings.h"

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
  virtual bool initConnections(IPluginManager * /*APluginManager*/, int &/*AInitOrder*/) { return true; }
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IXmppStreams
  virtual IXmppStream *newStream(const Jid &AStreamJid);
  virtual void addStream(IXmppStream *AXmppStream);
  virtual bool isActive(IXmppStream *AXmppStream) const { return FActiveStreams.contains(AXmppStream); }
  virtual IXmppStream *getStream(const Jid &AStreamJid) const;
  virtual QList<IXmppStream *> getStreams() const { return FStreams; }
  virtual void removeStream(IXmppStream *AXmppStream);
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
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamElement(IXmppStream *AXmppStream, const QDomElement &elem);
  void onStreamAboutToClose(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamError(IXmppStream *AXmppStream, const QString &AErrStr);
  void onStreamJidAboutToBeChanged(IXmppStream *AStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour);
private:
  QList<IXmppStream *> FStreams;
  QList<IXmppStream *> FActiveStreams;
};

#endif // XMPPSTREAMS_H
