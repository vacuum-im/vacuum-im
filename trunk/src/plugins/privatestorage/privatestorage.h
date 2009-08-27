#ifndef PRIVATESTORAGE_H
#define PRIVATESTORAGE_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iprivatestorage.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/stanza.h"

class PrivateStorage :
  public QObject,
  public IPlugin,
  public IPrivateStorage,
  public IStanzaRequestOwner
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IPrivateStorage IStanzaRequestOwner);
public:
  PrivateStorage();
  ~PrivateStorage();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return PRIVATESTORAGE_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IPrivateStorage
  virtual bool hasData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
  virtual QDomElement getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
  virtual QString saveData(const Jid &AStreamJid, const QDomElement &AElement);
  virtual QString loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
  virtual QString removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
signals:
  virtual void storageOpened(const Jid &AStreamJid);
  virtual void dataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  virtual void dataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  virtual void dataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  virtual void dataError(const QString &AId, const QString &AError);
  virtual void storageClosed(const Jid &AStreamJid);
protected:
  QDomElement getStreamElement(const Jid &AStreamJid);
  void removeStreamElement(const Jid &AStreamJid);
  QDomElement insertElement(const Jid &AStreamJid, const QDomElement &AElement);
  void removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  QDomDocument FStorage;
  QHash<Jid, QDomElement> FStreamElements;
  QHash<QString, QDomElement> FSaveRequests;
  QHash<QString, QDomElement> FLoadRequests;
  QHash<QString, QDomElement> FRemoveRequests;
};

#endif // PRIVATESTORAGE_H
