#ifndef ROSTERMODEL_H
#define ROSTERMODEL_H

#include "interfaces/irostermodel.h"
#include "utils/jid.h"
#include "rosterindex.h"

class RosterModel : 
  public IRosterModel
{
  Q_OBJECT;
  Q_INTERFACES(IRosterModel);

public:
  RosterModel(IRoster *ARoster, IPresence *APresence);
  ~RosterModel();

  virtual QObject* instance() { return this; }

  //QAbstractItemModel
  virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &AIndex) const;
  virtual bool hasChildren(const QModelIndex &AParent) const;
  virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
  virtual QVariant headerData(int ASection, Qt::Orientation AOrientation, int ARole = Qt::DisplayRole) const;

  //IRosterModel
  virtual IRosterIndex *rootIndex() const { return FRootIndex; }
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent);
  virtual IRosterIndex *createGroup(const QString &AName, int AType, IRosterIndex *AParent);
  virtual IRosterIndex *findRosterIndex(int AType, const QVariant &AId, IRosterIndex *AParent=0) const;
  virtual IRosterIndex *findGroup(const QString &AName, int AType, IRosterIndex *AParent=0) const;
  virtual bool removeRosterIndex(IRosterIndex *AIndex);
  virtual QString blankGroupName() const { return tr("Blank Group"); }
  virtual QString transportsGroupName() const { return tr("Transports"); }
  virtual QString myResourcesGroupName() const { return tr("My Resources"); }
signals:
  virtual void indexInserted(IRosterIndex *);
  virtual void indexRemoved(IRosterIndex *);
protected slots:
  void onRosterItemPush(IRosterItem *ARosterItem);
  void onRosterItemRemoved(IRosterItem *ARosterItem);
  void onSelfPresence(IPresence::Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid);
  void onPresenceItem(IPresenceItem *APresenceItem);
  void onIndexDataChanged(IRosterIndex *AIndex);
  void onIndexChildAboutToBeInserted(IRosterIndex *);
  void onIndexChildInserted(IRosterIndex *);
  void onIndexChildAboutToBeRemoved(IRosterIndex *);
  void onIndexChildRemoved(IRosterIndex *);
private:
  IRoster *FRoster;
  IPresence *FPresence;
private:
  RosterIndex *FRootIndex;
};

#endif // ROSTERMODEL_H
