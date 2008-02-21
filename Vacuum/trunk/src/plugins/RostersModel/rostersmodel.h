#ifndef ROSTERSMODEL_H
#define ROSTERSMODEL_H

#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersmodel.h"
#include "../../utils/jid.h"
#include "rosterindex.h"

class RostersModel : 
  public IRostersModel
{
  Q_OBJECT;
  Q_INTERFACES(IRostersModel);
public:
  RostersModel(QObject *AParent);
  ~RostersModel();
  virtual QObject *instance() { return this; }
  //QAbstractItemModel
  virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &AIndex) const;
  virtual bool hasChildren(const QModelIndex &AParent) const;
  virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual Qt::ItemFlags flags(const QModelIndex &AIndex) const; 
  virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
  //IRostersModel
  virtual IRosterIndex *addStream(IRoster *ARoster, IPresence *APresence);
  virtual QStringList streams() const { return FStreams.keys(); }
  virtual void removeStream(const QString &AStreamJid);
  virtual IRoster *getRoster(const QString &AStreamJid) const;
  virtual IPresence *getPresence(const QString &AStreamJid) const;
  virtual IRosterIndex *rootIndex() const { return FRootIndex; }
  virtual IRosterIndex *getStreamRoot(const Jid &AStreamJid) const;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent);
  virtual IRosterIndex *createGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent);
  virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent);
  virtual void removeRosterIndex(IRosterIndex *AIndex);
  virtual void insertDefaultDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual void removeDefaultDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual IRosterIndexList getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false);
  virtual IRosterIndex *findRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) const;
  virtual IRosterIndex *findGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent) const;
  virtual QModelIndex modelIndexByRosterIndex(IRosterIndex *AIndex);
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
  void onStreamJidChanged(IXmppStream *AXmppStream,const Jid &ABefour);
  void onRosterItemReceived(const IRosterItem &ARosterItem);
  void onRosterItemRemoved(const IRosterItem &ARosterItem);
  void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
  void onPresenceReceived(const IPresenceItem &APresenceItem);
  void onIndexDataChanged(IRosterIndex *AIndex, int ARole);
  void onIndexChildAboutToBeInserted(IRosterIndex *AIndex);
  void onIndexChildInserted(IRosterIndex *AIndex);
  void onIndexChildAboutToBeRemoved(IRosterIndex *AIndex);
  void onIndexChildRemoved(IRosterIndex *AIndex);
  void onIndexDestroyed(IRosterIndex *AIndex);
  void onDelayedDataChanged();
private:
  struct StreamItem {
    IRoster *roster;
    IPresence *presence;
    IRosterIndex *root;
  };
  RosterIndex *FRootIndex;
  QHash<QString,StreamItem> FStreams;
  QSet<IRosterIndex *> FChangedIndexes;
  QList<IRosterIndexDataHolder *> FDataHolders;
};

#endif // ROSTERSMODEL_H
