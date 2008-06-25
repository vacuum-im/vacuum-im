#include "rostersmodel.h"

#include <QtDebug>
#include <QTimer>

#define INDEX_CHANGES_FOR_RESET 20

RostersModel::RostersModel()
{
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FAccountManager = NULL;

  FRootIndex = new RosterIndex(RIT_Root,"IT_Root");
  FRootIndex->setParent(this);
  connect(FRootIndex,SIGNAL(dataChanged(IRosterIndex *, int)),
    SLOT(onIndexDataChanged(IRosterIndex *, int)));
  connect(FRootIndex,SIGNAL(childAboutToBeInserted(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childInserted(IRosterIndex *)),
    SLOT(onIndexChildInserted(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childRemoved(IRosterIndex *)),
    SLOT(onIndexChildRemoved(IRosterIndex *)));
}

RostersModel::~RostersModel()
{

}

void RostersModel::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Creating and handling roster tree");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Rosters Model"); 
  APluginInfo->uid = ROSTERSMODEL_UUID;
  APluginInfo->version = "0.1";
}

bool RostersModel::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      connect(xmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &))); 
    }
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &)),
        SLOT(onRosterItemReceived(IRoster *, const IRosterItem &))); 
      connect(FRosterPlugin->instance(),SIGNAL(rosterItemRemoved(IRoster *, const IRosterItem &)),
        SLOT(onRosterItemRemoved(IRoster *, const IRosterItem &)));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),
        SLOT(onPresenceChanged(IPresence *, int , const QString &, int)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)), 
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &))); 
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
    }
  }

  return true;
}

QModelIndex RostersModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());

  IRosterIndex *childIndex = parentIndex->child(ARow);

  if (childIndex)
    return createIndex(ARow,AColumn,childIndex);

  return QModelIndex();
}

QModelIndex RostersModel::parent(const QModelIndex &AIndex) const
{
  if (AIndex.isValid())
  {
    IRosterIndex *curIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
    IRosterIndex *parentIndex = curIndex->parentIndex();
    return modelIndexByRosterIndex(parentIndex); 
  }
  return QModelIndex();
}

bool RostersModel::hasChildren(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  return parentIndex->childCount() > 0;
}

int RostersModel::rowCount(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  return parentIndex->childCount();
}

int RostersModel::columnCount(const QModelIndex &/*AParent*/) const
{
  return 1;
}

Qt::ItemFlags RostersModel::flags(const QModelIndex &AIndex) const
{
  IRosterIndex *index = FRootIndex;
  if (AIndex.isValid())
    index = static_cast<IRosterIndex *>(AIndex.internalPointer());
  return index->flags();  
}

QVariant RostersModel::data(const QModelIndex &AIndex, int ARole) const 
{
  if (AIndex.isValid())
  {
    IRosterIndex *index = static_cast<IRosterIndex *>(AIndex.internalPointer());
    return index->data(ARole);
  }
  return FRootIndex->data(ARole);
}

IRosterIndex *RostersModel::addStream(const Jid &AStreamJid)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
  if (streamIndex == NULL)
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
    
    if (roster || presence)
    {
      IRosterIndex *streamIndex = createRosterIndex(RIT_StreamRoot,AStreamJid.pFull(),FRootIndex);
      streamIndex->setRemoveOnLastChildRemoved(false);
      streamIndex->setData(RDR_StreamJid,AStreamJid.pFull());
      streamIndex->setData(RDR_Jid,AStreamJid.full());
      streamIndex->setData(RDR_PJid,AStreamJid.pFull());
      streamIndex->setData(RDR_BareJid,AStreamJid.pBare());

      if (presence)
      {
        streamIndex->setData(RDR_Show, presence->show());
        streamIndex->setData(RDR_Status,presence->status());
      }
      if (account)
      {
        connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          SLOT(onAccountChanged(const QString &, const QVariant &)));
        streamIndex->setData(RDR_Name,account->name());
      }

      FStreamsRoot.insert(AStreamJid,streamIndex);
      insertRosterIndex(streamIndex,FRootIndex);
      emit streamAdded(AStreamJid);
      if (FRosterPlugin)
        FRosterPlugin->loadRosterItems(AStreamJid);
    }
  }
  return streamIndex;
}

void RostersModel::removeStream(const Jid &AStreamJid)
{
  IRosterIndex *streamIndex = FStreamsRoot.take(AStreamJid);
  if (streamIndex)
  {
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
    if (account)
    {
      disconnect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),
        this,SLOT(onAccountChanged(const QString &, const QVariant &)));
    }
    removeRosterIndex(streamIndex);
    emit streamRemoved(AStreamJid);
  }
}

IRosterIndex *RostersModel::streamRoot(const Jid &AStreamJid) const
{
  return FStreamsRoot.value(AStreamJid);
}

IRosterIndex *RostersModel::createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent)
{
  IRosterIndex *index = findRosterIndex(AType,AId,AParent);
  if (!index)
  {
    index = new RosterIndex(AType,AId);
    connect(index->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onIndexDestroyed(IRosterIndex *)));
    if (AParent)
      index->setData(RDR_StreamJid,AParent->data(RDR_StreamJid));
    emit indexCreated(index,AParent);
    insertDefaultDataHolders(index);
  }
  return index;
}

IRosterIndex *RostersModel::createGroup(const QString &AName, const QString &AGroupDelim, 
                                        int AType, IRosterIndex *AParent)
{
  IRosterIndex *index = findGroup(AName,AGroupDelim,AType,AParent);
  if (!index)
  {
    int i = 0;
    index = AParent;
    IRosterIndex *newIndex = (IRosterIndex *)true;
    QList<QString> groupTree = AName.split(AGroupDelim,QString::SkipEmptyParts); 
    
    QString group;
    if (AParent && AParent->data(RDR_Group).isValid())
      group = AParent->data(RDR_Group).toString();
    
    while (newIndex && i<groupTree.count())
    {
      if (group.isEmpty())
        group = groupTree.at(i);
      else
        group += AGroupDelim + groupTree.at(i);

      newIndex = findGroup(groupTree.at(i),AGroupDelim,AType,index);
      if (newIndex)
      {
        index = newIndex;
        i++;
      }
    }
    
    while (i<groupTree.count())
    {
      newIndex = createRosterIndex(AType,groupTree.at(i),index);
      newIndex->setData(RDR_Group,group);
      insertRosterIndex(newIndex,index);
      index = newIndex;
      group += AGroupDelim + groupTree.value(++i);
    }
  }
  return index;
}

void RostersModel::insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent)
{
  AIndex->setParentIndex(AParent);
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex)
{
  AIndex->setParentIndex(NULL);
}

IRosterIndexList RostersModel::getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate)
{
  IRosterIndexList indexList;
  IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
  if (streamIndex)
  {
    int type = RIT_Contact;
    if (AContactJid.node().isEmpty())
      type = RIT_Agent;
    else if (AContactJid && AStreamJid)
      type = RIT_MyResource;

    QHash<int,QVariant> data;
    data.insert(RDR_Type, type);
    if (AContactJid.resource().isEmpty())
      data.insert(RDR_BareJid, AContactJid.pBare());
    else
      data.insert(RDR_PJid, AContactJid.pFull());
    indexList = streamIndex->findChild(data,true);

    if (indexList.isEmpty() && !AContactJid.resource().isEmpty())
    {
      data.insert(RDR_PJid, AContactJid.pBare());
      indexList = streamIndex->findChild(data,true);
    }

    if (indexList.isEmpty() && ACreate)
    {
      IRosterIndex *group;
      if (type == RIT_MyResource)
        group = createGroup(myResourcesGroupName(),"::",RIT_MyResourcesGroup,streamIndex);
      else
        group = createGroup(notInRosterGroupName(),"::",RIT_NotInRosterGroup,streamIndex);
      IRosterIndex *index = createRosterIndex(type,AContactJid.pFull(),group);
      index->setData(RDR_Jid,AContactJid.full());
      index->setData(RDR_PJid,AContactJid.pFull());
      index->setData(RDR_BareJid,AContactJid.pBare());
      index->setData(RDR_Group,group->data(RDR_Group));
      insertRosterIndex(index,group);
      indexList.append(index);
    }
  }
  return indexList;
}

IRosterIndex *RostersModel::findRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) const
{
  QHash<int,QVariant> data;
  data.insert(RDR_Type,AType);  
  data.insert(RDR_Id,AId);
  if (AParent)
    return AParent->findChild(data).value(0,NULL);
  return FRootIndex->findChild(data).value(0,NULL) ;
}

IRosterIndex *RostersModel::findGroup(const QString &AName, const QString &AGroupDelim, 
                                      int AType, IRosterIndex *AParent) const
{
  int i = 0;
  IRosterIndex *index = AParent;
  QList<QString> groupTree = AName.split(AGroupDelim,QString::SkipEmptyParts);
  do {
    index = findRosterIndex(AType,groupTree.at(i++),index);  
  } while (index && i<groupTree.count());
  return index;
}

void RostersModel::insertDefaultDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  if (ADataHolder && !FDataHolders.contains(ADataHolder))
  {
    QMultiHash<int,QVariant> data;
    foreach(int type, ADataHolder->types())
      data.insertMulti(RDR_Type,type);

    QList<IRosterIndex *> indexes = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index,indexes)
      index->insertDataHolder(ADataHolder);

    FDataHolders.append(ADataHolder);
    emit defaultDataHolderInserted(ADataHolder);
  }
}

void RostersModel::removeDefaultDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  if (ADataHolder && FDataHolders.contains(ADataHolder))
  {
    QMultiHash<int,QVariant> data;
    foreach(int type, ADataHolder->types())
      data.insertMulti(RDR_Type,type);

    QList<IRosterIndex *> indexes = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index,indexes)
      index->removeDataHolder(ADataHolder);

    FDataHolders.removeAt(FDataHolders.indexOf(ADataHolder));
    emit defaultDataHolderRemoved(ADataHolder);
  }
}

QModelIndex RostersModel::modelIndexByRosterIndex(IRosterIndex *AIndex) const
{
  return AIndex!=NULL && AIndex!=FRootIndex ? createIndex(AIndex->row(),0,AIndex) : QModelIndex();
}

IRosterIndex *RostersModel::rosterIndexByModelIndex(const QModelIndex &AIndex) const
{
  return static_cast<IRosterIndex *>(AIndex.internalPointer());
}

void RostersModel::emitDelayedDataChanged(IRosterIndex *AIndex)
{
  FChangedIndexes -= AIndex;

  if (AIndex!=FRootIndex)
  {
    QModelIndex modelIndex = modelIndexByRosterIndex(AIndex);
    emit dataChanged(modelIndex,modelIndex);
  }

  IRosterIndexList childs;
  foreach(IRosterIndex *index, FChangedIndexes)
    if (index->parentIndex() == AIndex)
      childs.append(index);

  foreach(IRosterIndex *index, childs)
    emitDelayedDataChanged(index);
}

void RostersModel::insertDefaultDataHolders(IRosterIndex *AIndex)
{
  foreach(IRosterIndexDataHolder *dataHolder, FDataHolders)
    if (dataHolder->types().contains(AIndex->type()))
      AIndex->insertDataHolder(dataHolder);
}

void RostersModel::onAccountShown(IAccount *AAccount)
{
  addStream(AAccount->streamJid());
}

void RostersModel::onAccountHidden(IAccount *AAccount)
{
  removeStream(AAccount->streamJid());
}

void RostersModel::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == AVN_NAME)
  {
    IAccount *account = qobject_cast<IAccount *>(sender());
    if (account)
    {
      IRosterIndex *streamIndex = FStreamsRoot.value(account->streamJid());
      if (streamIndex)
        streamIndex->setData(RDR_Name,AValue.toString());
    }
  }
}

void RostersModel::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(ABefour);
  if (streamIndex)
  {
    Jid after = AXmppStream->jid();

    QHash<int,QVariant> data;
    data.insert(RDR_StreamJid,ABefour.pFull());
    IRosterIndexList itemList = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index, itemList)
      index->setData(RDR_StreamJid,after.pFull());

    streamIndex->setData(RDR_Id,after.pFull());
    streamIndex->setData(RDR_Jid,after.full());
    streamIndex->setData(RDR_PJid,after.pFull());
    
    FStreamsRoot.remove(ABefour);
    FStreamsRoot.insert(after,streamIndex);

    emit streamJidChanged(ABefour,after);
  }
}

void RostersModel::onRosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(ARoster->streamJid());
  if (streamIndex)
  {
    int groupType; 
    QString groupDisplay;
    QSet<QString> itemGroups; 
    int itemType = !ARosterItem.itemJid.node().isEmpty() ? RIT_Contact : RIT_Agent;

    if (itemType == RIT_Agent)
    {
      groupType = RIT_AgentsGroup;
      groupDisplay = agentsGroupName();
      itemGroups.insert(""); 
    }
    else if (ARosterItem.groups.isEmpty())
    {
      groupType = RIT_BlankGroup; 
      groupDisplay = blankGroupName();
      itemGroups.insert(""); 
    }
    else
    {
      groupType = RIT_Group;
      itemGroups = ARosterItem.groups;
    }

    QHash<int,QVariant> findData;
    findData.insert(RDR_Type,itemType);
    findData.insert(RDR_BareJid,ARosterItem.itemJid.pBare());     
    IRosterIndexList curItemList = streamIndex->findChild(findData,true);

    QSet<QString> curGroups;
    foreach(IRosterIndex *index, curItemList)
      curGroups.insert(index->data(RDR_Group).toString());
    QSet<QString> newGroups = itemGroups - curGroups;
    QSet<QString> oldGroups = curGroups - itemGroups; 

    IRosterIndexList itemList; 
    QString groupDelim = ARoster->groupDelimiter();
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(ARoster->streamJid()) : NULL;
    foreach(QString group,itemGroups)
    {
      IRosterIndex *groupIndex = createGroup(groupType==RIT_Group ? group : groupDisplay,groupDelim,groupType,streamIndex);

      IRosterIndexList groupItemList;
      //Если есть возможность переносим контакты из старой группы в новую
      if (newGroups.contains(group) && !oldGroups.isEmpty())
      {
        IRosterIndex *oldGroupIndex;
        QString oldGroup = oldGroups.values().value(0);
        if (oldGroup.isEmpty())
          oldGroupIndex = findGroup(blankGroupName(),groupDelim,RIT_BlankGroup,streamIndex);
        else
          oldGroupIndex = findGroup(oldGroup,groupDelim,RIT_Group,streamIndex);
        if (oldGroupIndex)
        {
          groupItemList = oldGroupIndex->findChild(findData);
          foreach(IRosterIndex *index,groupItemList)
          {
            index->setData(RDR_Group,group);  
            index->setParentIndex(groupIndex);
          }
        }
      }
      else
        groupItemList = groupIndex->findChild(findData);

      //Если в этой группе нет контактов, то создаем их
      if (groupItemList.isEmpty())
      {
        int presIndex = 0;
        QList<IPresenceItem> pitems = presence!=NULL ? presence->presenceItems(ARosterItem.itemJid) : QList<IPresenceItem>();
        do 
        {
          IRosterIndex *index;
          IPresenceItem pitem = pitems.value(presIndex++);
          if (pitem.isValid)
          {
            index = createRosterIndex(itemType,pitem.itemJid.pFull(),groupIndex);
            index->setData(RDR_Jid,pitem.itemJid.full());
            index->setData(RDR_PJid,pitem.itemJid.pFull());
            index->setData(RDR_Show,pitem.show);
            index->setData(RDR_Status,pitem.status);
            index->setData(RDR_Priority,pitem.priority);
          }
          else
          {
            index = createRosterIndex(itemType,ARosterItem.itemJid.pBare(),groupIndex);
            index->setData(RDR_Jid,ARosterItem.itemJid.bare());
            index->setData(RDR_PJid,ARosterItem.itemJid.pBare());
          }

          index->setData(RDR_BareJid,ARosterItem.itemJid.pBare());
          index->setData(RDR_Name,ARosterItem.name); 
          index->setData(RDR_Subscription,ARosterItem.subscription);
          index->setData(RDR_Ask,ARosterItem.ask);
          index->setData(RDR_Group,group); 

          itemList.append(index);
          insertRosterIndex(index,groupIndex);
        } 
        while(presIndex < pitems.count());
      }
      else foreach(IRosterIndex *index,groupItemList)
      {
        index->setData(RDR_Name,ARosterItem.name); 
        index->setData(RDR_Subscription,ARosterItem.subscription);
        index->setData(RDR_Ask,ARosterItem.ask);
        itemList.append(index);
      }
    }

    //Удаляем контакты из старых групп
    foreach(IRosterIndex *index,curItemList)
      if (!itemList.contains(index))
        removeRosterIndex(index);
  }
}

void RostersModel::onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(ARoster->streamJid());
  if (streamIndex)
  {
    QMultiHash<int,QVariant> data;
    data.insert(RDR_Type,RIT_Contact);     
    data.insert(RDR_Type,RIT_Agent);     
    data.insert(RDR_BareJid,ARosterItem.itemJid.pBare());     
    IRosterIndexList itemList = streamIndex->findChild(data,true);
    foreach(IRosterIndex *index, itemList)
      removeRosterIndex(index);    
  }
}

void RostersModel::onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
  if (streamIndex)
  {
    streamIndex->setData(RDR_Show, AShow);
    streamIndex->setData(RDR_Status, AStatus);
    if (AShow != IPresence::Offline && AShow != IPresence::Error)
      streamIndex->setData(RDR_Priority, APriority);
    else
      streamIndex->setData(RDR_Priority, QVariant());
  }
}

void RostersModel::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
  if (streamIndex)
  {
    int itemType = RIT_Contact;
    if (APresenceItem.itemJid.node().isEmpty())
      itemType = RIT_Agent; 
    else if (APresenceItem.itemJid && APresence->streamJid())
      itemType = RIT_MyResource; 

    if (APresenceItem.show == IPresence::Offline)
    {
      QHash<int,QVariant> findData;
      findData.insert(RDR_Type,itemType);
      findData.insert(RDR_PJid,APresenceItem.itemJid.pFull());
      IRosterIndexList itemList = streamIndex->findChild(findData,true);
      int pitemsCount = APresence->presenceItems(APresenceItem.itemJid).count();
      foreach (IRosterIndex *index,itemList)
      {
        if (itemType == RIT_MyResource || pitemsCount > 1)
        {
          removeRosterIndex(index);
        }
        else
        {
          index->setData(RDR_Id,APresenceItem.itemJid.pBare());
          index->setData(RDR_Jid,APresenceItem.itemJid.bare());
          index->setData(RDR_PJid,APresenceItem.itemJid.pBare());
          index->setData(RDR_Show,APresenceItem.show);
          index->setData(RDR_Status,APresenceItem.status);
          index->setData(RDR_Priority,QVariant()); 
        }
      }
    }
    else if (APresenceItem.show == IPresence::Error)
    {
      QHash<int,QVariant> findData;
      findData.insert(RDR_Type,itemType);
      findData.insert(RDR_BareJid,APresenceItem.itemJid.pBare());
      IRosterIndexList itemList = streamIndex->findChild(findData,true);
      foreach(IRosterIndex *index,itemList)
      {
        index->setData(RDR_Show,APresenceItem.show);
        index->setData(RDR_Status,APresenceItem.status);
        index->setData(RDR_Priority,QVariant()); 
      }
    }
    else
    {
      QHash<int,QVariant> findData;
      findData.insert(RDR_Type,itemType);
      findData.insert(RDR_PJid,APresenceItem.itemJid.pFull());
      IRosterIndexList itemList = streamIndex->findChild(findData,true);
      if (itemList.isEmpty())
      {
        IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(APresence->streamJid()) : NULL;
        IRosterItem ritem = roster!=NULL ? roster->rosterItem(APresenceItem.itemJid) : IRosterItem();
        QString groupDelim = roster!=NULL ? roster->groupDelimiter() : "::";

        QSet<QString> itemGroups;
        if (ritem.isValid)
          itemGroups = ritem.groups;
        if (itemGroups.isEmpty())
          itemGroups.insert(""); 

        foreach(QString group,itemGroups)
        {
          IRosterIndex *groupIndex = NULL;
          if (itemType == RIT_MyResource)
            groupIndex = createGroup(myResourcesGroupName(),groupDelim,RIT_MyResourcesGroup,streamIndex);
          else if (!ritem.isValid)
            groupIndex = createGroup(notInRosterGroupName(),groupDelim,RIT_NotInRosterGroup,streamIndex);
          else if (itemType == RIT_Agent)
            groupIndex = findGroup(agentsGroupName(),groupDelim,RIT_AgentsGroup,streamIndex);
          else if (group.isEmpty()) 
            groupIndex = findGroup(blankGroupName(),groupDelim,RIT_BlankGroup,streamIndex);
          else
            groupIndex = findGroup(group,groupDelim,RIT_Group,streamIndex);

          if (groupIndex)
          {
            IRosterIndex *index = findRosterIndex(itemType,APresenceItem.itemJid.pBare(),groupIndex);   
            if (!index)
            {
              index = createRosterIndex(itemType,APresenceItem.itemJid.pFull(),groupIndex);
              index->setData(RDR_BareJid,APresenceItem.itemJid.pBare());
              index->setData(RDR_Group,group); 
              if (ritem.isValid)
              {
                index->setData(RDR_Name,ritem.name); 
                index->setData(RDR_Subscription,ritem.subscription);
                index->setData(RDR_Ask,ritem.ask);
              }
            }
            else
              index->setData(RDR_Id,APresenceItem.itemJid.pFull());

            index->setData(RDR_Jid,APresenceItem.itemJid.full());
            index->setData(RDR_PJid,APresenceItem.itemJid.pFull());
            index->setData(RDR_Show,APresenceItem.show);
            index->setData(RDR_Status,APresenceItem.status);
            index->setData(RDR_Priority,APresenceItem.priority);
            insertRosterIndex(index,groupIndex);
          }
        }
      }
      else foreach(IRosterIndex *index,itemList)
      {
        index->setData(RDR_Show,APresenceItem.show);
        index->setData(RDR_Status,APresenceItem.status);
        index->setData(RDR_Priority,APresenceItem.priority);
      }
    }
  }
}

void RostersModel::onIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
  if (FChangedIndexes.isEmpty())
    QTimer::singleShot(0,this,SLOT(onDelayedDataChanged()));
  FChangedIndexes+=AIndex;
  emit indexDataChanged(AIndex, ARole);
}

void RostersModel::onIndexChildAboutToBeInserted(IRosterIndex *AIndex)
{
  beginInsertRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->row(),AIndex->row());
  connect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    SLOT(onIndexDataChanged(IRosterIndex *, int)));
  connect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
  connect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
    SLOT(onIndexChildInserted(IRosterIndex *)));
  connect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
  connect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
    SLOT(onIndexChildRemoved(IRosterIndex *)));
}

void RostersModel::onIndexChildInserted(IRosterIndex *AIndex)
{
  endInsertRows();
  emit indexInserted(AIndex);
}

void RostersModel::onIndexChildAboutToBeRemoved(IRosterIndex *AIndex)
{
  FChangedIndexes-=AIndex;
  emit indexRemoved(AIndex);
  beginRemoveRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->row(),AIndex->row());
}

void RostersModel::onIndexChildRemoved(IRosterIndex *AIndex)
{
  disconnect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    this,SLOT(onIndexDataChanged(IRosterIndex *, int)));
  disconnect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
    this,SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
    this,SLOT(onIndexChildInserted(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
    this,SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
    this,SLOT(onIndexChildRemoved(IRosterIndex *)));
  endRemoveRows();
}

void RostersModel::onIndexDestroyed(IRosterIndex *AIndex)
{
  emit indexDestroyed(AIndex);
}

void RostersModel::onDelayedDataChanged()
{
  //Замена сигналов изменения большого числа индексов на сброс модели
  //из-за тормозов в сортировке в QSortFilterProxyModel
  if (FChangedIndexes.count() < INDEX_CHANGES_FOR_RESET)
  {
    //Вызывает dataChanged у всех родителей для поддержки SortFilterProxyModel
    QSet<IRosterIndex *> childIndexes = FChangedIndexes;
    foreach(IRosterIndex *index,childIndexes)
    {
      IRosterIndex *parentIndex = index->parentIndex();
      while (parentIndex && !FChangedIndexes.contains(parentIndex))
      {
        FChangedIndexes+=parentIndex;
        parentIndex = parentIndex->parentIndex();
      }
      QModelIndex modelIndex = modelIndexByRosterIndex(index);
      emit dataChanged(modelIndex,modelIndex);
    }
    emitDelayedDataChanged(FRootIndex);
  }
  else
    reset();

  FChangedIndexes.clear();
}

Q_EXPORT_PLUGIN2(RostersModelPlugin, RostersModel)
