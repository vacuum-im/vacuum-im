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
  enum {
    IT_Root,
    IT_StreamRoot,
    IT_Group,
    IT_BlankGroup,
    IT_TransportsGroup,
    IT_MyResourcesGroup,
    IT_NotInRosterGroup,
    IT_Contact,
    IT_Transport,
    IT_MyResource,
    IT_UserDefined = 32
  };
  enum {
    DR_AnyRole = -1,
    DR_Type = Qt::UserRole, 
    DR_StreamJid,
    DR_Id,
    DR_Jid,
    DR_RosterJid,
    DR_RosterGroup,
    DR_Show,
    DR_Status,
    DR_Priority,
    DR_Self_Show,
    DR_Self_Status,
    DR_Self_Priority,
    DR_Subscription,
    DR_Ask,
    DR_UserDefined = Qt::UserRole+32
  };
public:
  virtual QObject *instance() =0;
  virtual int type() const =0;
  virtual QString id() const =0;
  virtual void setParentIndex(IRosterIndex *) =0;
  virtual IRosterIndex *parentIndex() const =0; 
  virtual int row() const =0;
  virtual IRosterIndexDataHolder *setDataHolder(int ARole, IRosterIndexDataHolder *) =0;
  virtual void appendChild(IRosterIndex *) =0;
  virtual bool removeChild(IRosterIndex *, bool ARecurse = false) =0;
  virtual IRosterIndex *child(int ARow) const =0;
  virtual int childCount() const =0;
  virtual int childRow(const IRosterIndex *) const =0;
  virtual void setFlags(const Qt::ItemFlags &AFlags) =0;
  virtual Qt::ItemFlags flags() const =0;
  virtual bool setData(int ARole, const QVariant &) =0;
  virtual QVariant data(int ARole) const =0;
  virtual IRosterIndexList findChild(const QHash<int, QVariant> AData, bool ARecurse = false) const =0;
  virtual void setRemoveOnLastChildRemoved(bool ARemove) =0;
signals:
  virtual void dataChanged(IRosterIndex *) =0;
  virtual void childAboutToBeInserted(IRosterIndex *) =0;
  virtual void childInserted(IRosterIndex *) =0;
  virtual void childAboutToBeRemoved(IRosterIndex *) =0;
  virtual void childRemoved(IRosterIndex *) =0;
};

class IRostersModel :
  virtual public QAbstractItemModel
{
public:
  virtual QObject *instance() =0;
  virtual IRosterIndex *appendStream(IRoster *ARoster, IPresence *APresence) =0;
  virtual QStringList streams() const =0;
  virtual bool removeStream(const QString &AStreamJid) =0;
  virtual IRoster *getRoster(const QString &AStreamJid) const =0;
  virtual IPresence *getPresence(const QString &AStreamJid) const =0;
  virtual IRosterIndex *getStreamRoot(const QString &AStreamJid) const =0;
  virtual IRosterIndex *rootIndex() const =0;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *) =0;
  virtual IRosterIndex *createGroup(const QString &AName, int AType, IRosterIndex *) =0;
  virtual IRosterIndex *findRosterIndex(int AType, const QVariant &AId, IRosterIndex *) const =0;
  virtual IRosterIndex *findGroup(const QString &AName, int AType, IRosterIndex *) const =0;
  virtual bool removeRosterIndex(IRosterIndex *) =0;
  virtual QString blankGroupName() const =0;
  virtual QString transportsGroupName() const =0;
  virtual QString myResourcesGroupName() const =0;
  virtual QString notInRosterGroupName() const =0;
signals:
  virtual void streamAdded(const Jid &) =0;
  virtual void streamRemoved(const Jid &) =0;
  virtual void indexInserted(IRosterIndex *) =0;
  virtual void indexChanged(IRosterIndex *) =0;
  virtual void indexRemoved(IRosterIndex *) =0;
};


class IRostersModelPlugin 
{
public:
  virtual QObject *instance() =0;
  virtual IRostersModel *rostersModel() =0;
  virtual bool addStreamRoster(IRoster *, IPresence *) =0;
  virtual bool removeStreamRoster(const Jid &AStreamJid) =0;
signals:
  virtual void streamRosterAdded(const Jid &AStreamJid) =0;
  virtual void streamRosterRemoved(const Jid &AStreamJid) =0;
};

Q_DECLARE_INTERFACE(IRosterIndexDataHolder,"Vacuum.Plugin.IRosterIndexDataHolder/1.0");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.0");
Q_DECLARE_INTERFACE(IRostersModel,"Vacuum.Plugin.IRostersModel/1.0");
Q_DECLARE_INTERFACE(IRostersModelPlugin,"Vacuum.Plugin.IRostersModelPlugin/1.0");

#endif