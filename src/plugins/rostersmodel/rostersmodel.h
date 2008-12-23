#ifndef ROSTERSMODEL_H
#define ROSTERSMODEL_H

#include "../../definations/accountvaluenames.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../utils/jid.h"
#include "rosterindex.h"

class RostersModel : 
  virtual public QAbstractItemModel,
  public IPlugin,
  public IRostersModel
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRostersModel);
public:
  RostersModel();
  ~RostersModel();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERSMODEL_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //QAbstractItemModel
  virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &AIndex) const;
  virtual bool hasChildren(const QModelIndex &AParent) const;
  virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual Qt::ItemFlags flags(const QModelIndex &AIndex) const; 
  virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
  //IRostersModel
  virtual IRosterIndex *addStream(const Jid &AStreamJid);
  virtual QList<Jid> streams() const { return FStreamsRoot.keys(); }
  virtual void removeStream(const Jid &AStreamJid);
  virtual IRosterIndex *rootIndex() const { return FRootIndex; }
  virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent);
  virtual IRosterIndex *createGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent);
  virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent);
  virtual void removeRosterIndex(IRosterIndex *AIndex);
  virtual IRosterIndexList getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false);
  virtual IRosterIndex *findRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) const;
  virtual IRosterIndex *findGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent) const;
  virtual void insertDefaultDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual void removeDefaultDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual QModelIndex modelIndexByRosterIndex(IRosterIndex *AIndex) const;
  virtual IRosterIndex *rosterIndexByModelIndex(const QModelIndex &AIndex) const;
  virtual QString blankGroupName() const { return tr("Blank Group"); }
  virtual QString agentsGroupName() const { return tr("Agents"); }
  virtual QString myResourcesGroupName() const { return tr("My Resources"); }
  virtual QString notInRosterGroupName() const { return tr("Not in Roster"); }
signals:
  virtual void streamAdded(const Jid &AStreamJid);
  virtual void streamRemoved(const Jid &AStreamJid);
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAfter);
  virtual void indexCreated(IRosterIndex *AIndex, IRosterIndex *AParent);
  virtual void indexInserted(IRosterIndex *AIndex);
  virtual void indexDataChanged(IRosterIndex *AIndex, int ARole);
  virtual void indexRemoved(IRosterIndex *AIndex);
  virtual void indexDestroyed(IRosterIndex *AIndex);
  virtual void defaultDataHolderInserted(IRosterIndexDataHolder *ADataHolder);
  virtual void defaultDataHolderRemoved(IRosterIndexDataHolder *ADataHolder);
protected:
  void emitDelayedDataChanged(IRosterIndex *AIndex);
  void insertDefaultDataHolders(IRosterIndex *AIndex);
protected slots:
  void onAccountShown(IAccount *AAccount);
  void onAccountHidden(IAccount *AAccount);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
  void onStreamJidChanged(IXmppStream *AXmppStream,const Jid &ABefour);
  void onRosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem);
  void onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem);
  void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
  void onIndexDataChanged(IRosterIndex *AIndex, int ARole);
  void onIndexChildAboutToBeInserted(IRosterIndex *AIndex);
  void onIndexChildInserted(IRosterIndex *AIndex);
  void onIndexChildAboutToBeRemoved(IRosterIndex *AIndex);
  void onIndexChildRemoved(IRosterIndex *AIndex);
  void onIndexDestroyed(IRosterIndex *AIndex);
  void onDelayedDataChanged();
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IAccountManager *FAccountManager;
private:
  RosterIndex *FRootIndex;
  QHash<Jid,IRosterIndex *> FStreamsRoot;
  QSet<IRosterIndex *> FChangedIndexes;
  QList<IRosterIndexDataHolder *> FDataHolders;
};

#endif // ROSTERSMODEL_H
