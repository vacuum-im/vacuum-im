#ifndef IROSTERMODEL_H
#define IROSTERMODEL_H

#include <QVariant>
#include <QAbstractItemModel>
#include <QAbstractItemDelegate>
#include <QHash>
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"

#define ROSTERSMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

class IRosterIndex;
typedef QList<IRosterIndex *> IRosterIndexList;

class IRosterIndexDataHolder
{
public:
  virtual QObject *instance() =0;
  virtual bool setData(IRosterIndex *, int ARole, const QVariant &) =0;
  virtual QVariant data(const IRosterIndex *, int ARole) const =0;
  virtual QList<int> roles() const =0;
signals:
  virtual void dataChanged(IRosterIndex *, int ARole) =0;
};

class IRosterIndex
{
public:
  enum ItemType {
    IT_Root,
    IT_StreamRoot,
    IT_Group,
    IT_BlankGroup,
    IT_NotInRosterGroup,
    IT_MyResourcesGroup,
    IT_AgentsGroup,
    IT_Contact,
    IT_Agent,
    IT_MyResource,
    IT_UserDefined = 64,
    IT_UserDynamic = IT_UserDefined + 1048576
  };
  enum DataRole{
    DR_AnyRole = -1,
    DR_Type = Qt::UserRole, 
    DR_Id,
    //XMPP Roles
    DR_StreamJid,
    DR_Jid,
    DR_RosterJid,
    DR_RosterGroup,
    DR_RosterName,
    DR_GroupName,
    DR_Show,
    DR_Status,
    DR_Priority,
    DR_Self_Show,
    DR_Self_Status,
    DR_Self_Priority,
    DR_Subscription,
    DR_Ask,
    //Decoration
    DR_ShowGroupExpander,
    DR_FontHint,
    DR_FontSize,
    DR_FontWeight,
    DR_FontStyle,
    DR_FontUnderline,
    //User roles
    DR_UserDefined = Qt::UserRole + 64,
    DR_UserDynamic = DR_UserDefined + 1048576
  };
public:
  virtual QObject *instance() =0;
  virtual int type() const =0;
  virtual int newType() const {FNewType++; return FNewType;}
  virtual QString id() const =0;
  virtual void setParentIndex(IRosterIndex *) =0;
  virtual IRosterIndex *parentIndex() const =0; 
  virtual int row() const =0;
  virtual IRosterIndexDataHolder *setDataHolder(int ARole, IRosterIndexDataHolder *) =0;
  virtual QHash<int,IRosterIndexDataHolder *> setDataHolder(IRosterIndexDataHolder *) =0;
  virtual void appendChild(IRosterIndex *) =0;
  virtual bool removeChild(IRosterIndex *) =0;
  virtual IRosterIndex *child(int ARow) const =0;
  virtual int childCount() const =0;
  virtual int childRow(const IRosterIndex *) const =0;
  virtual void setFlags(const Qt::ItemFlags &AFlags) =0;
  virtual Qt::ItemFlags flags() const =0;
  virtual int newRole() const {FNewRole++; return FNewRole;}
  virtual bool setData(int ARole, const QVariant &) =0;
  virtual QVariant data(int ARole) const =0;
  virtual void setItemDelegate(QAbstractItemDelegate *AItemDelegate) =0;
  virtual QAbstractItemDelegate *itemDelegate() const =0;
  virtual IRosterIndexList findChild(const QMultiHash<int, QVariant> AData, bool ASearchInChilds = false) const =0;
  virtual void setRemoveOnLastChildRemoved(bool ARemove) =0;
  virtual void setRemoveChildsOnRemoved(bool ARemove) =0;
  virtual void setDestroyOnParentRemoved(bool ADestroy) =0;
signals:
  virtual void dataChanged(IRosterIndex *, int ARole = DR_AnyRole) =0;
  virtual void childAboutToBeInserted(IRosterIndex *) =0;
  virtual void childInserted(IRosterIndex *) =0;
  virtual void childAboutToBeRemoved(IRosterIndex *) =0;
  virtual void childRemoved(IRosterIndex *) =0;
private:
  static int FNewType;
  static int FNewRole;
};

class IRostersModel :
  virtual public QAbstractItemModel
{
public:
  virtual QObject *instance() =0;
  virtual IRosterIndex *appendStream(IRoster *ARoster, IPresence *APresence) =0;
  virtual QStringList streams() const =0;
  virtual void removeStream(const QString &AStreamJid) =0;
  virtual IRoster *getRoster(const QString &AStreamJid) const =0;
  virtual IPresence *getPresence(const QString &AStreamJid) const =0;
  virtual IRosterIndex *getStreamRoot(const Jid &AStreamJid) const =0;
  virtual IRosterIndex *rootIndex() const =0;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) =0;
  virtual IRosterIndex *createGroup(const QString &AName, const QString &AGroupDelim, 
    int AType, IRosterIndex *AParent) =0;
  virtual IRosterIndex *findRosterIndex(int AType, const QVariant &AId, IRosterIndex *AParent) const =0;
  virtual IRosterIndex *findGroup(const QString &AName, const QString &AGroupDelim, 
    int AType, IRosterIndex *AParent) const =0;
  virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
  virtual void removeRosterIndex(IRosterIndex *AIndex) =0;
  virtual QModelIndex modelIndexByRosterIndex(IRosterIndex *AIndex) =0;
  virtual QString blankGroupName() const =0;
  virtual QString agentsGroupName() const =0;
  virtual QString myResourcesGroupName() const =0;
  virtual QString notInRosterGroupName() const =0;
signals:
  virtual void streamAdded(const Jid &) =0;
  virtual void streamRemoved(const Jid &) =0;
  virtual void indexCreated(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
  virtual void indexInserted(IRosterIndex *) =0;
  virtual void indexDataChanged(IRosterIndex *, int ARole) =0;
  virtual void indexRemoved(IRosterIndex *) =0;
};

class IRostersModelPlugin 
{
public:
  virtual QObject *instance() =0;
  virtual IRostersModel *rostersModel() =0;
  virtual IRosterIndex *addStream(IRoster *, IPresence *) =0;
  virtual void removeStream(const Jid &AStreamJid) =0;
signals:
  virtual void modelCreated(IRostersModel *) =0;
  virtual void modelDestroyed(IRostersModel *) =0;
};

Q_DECLARE_INTERFACE(IRosterIndexDataHolder,"Vacuum.Plugin.IRosterIndexDataHolder/1.0");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.0");
Q_DECLARE_INTERFACE(IRostersModel,"Vacuum.Plugin.IRostersModel/1.0");
Q_DECLARE_INTERFACE(IRostersModelPlugin,"Vacuum.Plugin.IRostersModelPlugin/1.0");

#endif