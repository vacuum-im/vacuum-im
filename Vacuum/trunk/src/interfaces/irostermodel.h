#ifndef IROSTERMODEL_H
#define IROSTERMODEL_H

#include <QVariant>
#include <QAbstractItemModel>
#include <QHash>
#include "interfaces/iroster.h"
#include "interfaces/ipresence.h"

class IRosterIndex;
typedef QList<IRosterIndex *> IRosterIndexList;

class IRosterIndexDataHolder
{
public:
  virtual QObject *instance() =0;
  virtual bool setData(IRosterIndex *, int ARole, const QVariant &) =0;
  virtual QVariant data(const IRosterIndex *, int ARole) const =0;
signals:
  virtual void dataChanged() =0;
};

class IRosterIndex
{
public:
  enum IndexType {
    IT_Root,
    IT_Group,
    IT_BlankGroup,
    IT_MyResourcesGroup,
    IT_TransportsGroup,
    IT_Contact,
    IT_Transport,
    IT_MyResource,
    IT_UserDefined = 16
  };
  enum DataRole {
    DR_Type = Qt::UserRole, 
    DR_Id,
    DR_StreamJid,
    DR_RosterJid,
    DR_RosterGroup,
    DR_Jid,
    DR_Show,
    DR_Status,
    DR_Priority,
    DR_Subscription,
    DR_Ask,
    DR_UserDefined = Qt::UserRole+32
  };
public:
  virtual QObject *instance() =0;
  virtual int type() const =0;
  virtual void setParentIndex(IRosterIndex *) =0;
  virtual IRosterIndex *parentIndex() const =0; 
  virtual int row() const =0;
  virtual bool setData(int ARole, const QVariant &) =0;
  virtual QVariant data(int ARole) const =0;
  virtual IRosterIndexDataHolder *setDataHolder(int ARole, IRosterIndexDataHolder *) =0;
  virtual void appendChild(IRosterIndex *) =0;
  virtual bool removeChild(IRosterIndex *, bool ARecurse = false) =0;
  virtual IRosterIndex *child(int ARow) const =0;
  virtual int childCount() const =0;
  virtual int childRow(const IRosterIndex *) const =0;
  virtual IRosterIndexList findChild(const QHash<int, QVariant> AData, bool ARecurse = false) const =0;
signals:
  virtual void dataChanged(IRosterIndex *) =0;
};

class IRosterModel :
  public QAbstractItemModel
{
public:
  virtual QObject *instance() =0;
  virtual IRosterIndex *rootIndex() const =0;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *) =0;
  virtual IRosterIndex *createGroup(const QString &AName, IRosterIndex *) =0;
  virtual IRosterIndex *createContact(const Jid &, IRosterIndex *) =0;
  virtual IRosterIndex *findRosterIndex(int AType, const QVariant &AId, IRosterIndex *) const=0;
  virtual IRosterIndex *findGroup(const QString &AName, IRosterIndex *) const=0;
  virtual bool removeRosterIndex(IRosterIndex *) =0;
};

class IRosterModelPlugin 
{
public:
  virtual QObject *instance() =0;
  virtual IRosterModel *newRosterModel(IRoster *, IPresence *) =0;
  virtual IRosterModel *getRosterModel(const Jid &) const =0;
  virtual void removeRosterModel(const Jid &) =0;
signals:
  virtual void rosterModelAdded(IRosterModel *) =0;
  virtual void rosterModelRemoved(IRosterModel *) =0;
};

Q_DECLARE_INTERFACE(IRosterIndexDataHolder,"Vacuum.Plugin.IRosterIndexDataHolder/1.0");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.0");
Q_DECLARE_INTERFACE(IRosterModel,"Vacuum.Plugin.IRosterModel/1.0");
Q_DECLARE_INTERFACE(IRosterModelPlugin,"Vacuum.Plugin.IRosterModelPlugin/1.0");

#endif