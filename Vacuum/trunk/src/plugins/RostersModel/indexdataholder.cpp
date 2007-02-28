#include "indexdataholder.h"

IndexDataHolder::IndexDataHolder(QObject *AParent) :
  QObject(AParent)
{
  FStatusIconset.openFile("status/default.jisp");
}

IndexDataHolder::~IndexDataHolder()
{

}

bool IndexDataHolder::setData(IRosterIndex *, int , const QVariant &)
{
  return false;
}

QVariant IndexDataHolder::data(const IRosterIndex *AIndex, int ARole) const
{
  switch (AIndex->type())
  {
  case IRosterIndex::IT_StreamRoot:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(IRosterIndex::DR_Jid);
    case Qt::DecorationRole: 
      return statusIcon(AIndex);
    case Qt::BackgroundColorRole:
      return Qt::darkGray;
    case Qt::ForegroundRole:
      return Qt::white;
    } 
    break;
  
  case IRosterIndex::IT_Group:
  case IRosterIndex::IT_BlankGroup:
  case IRosterIndex::IT_TransportsGroup:
  case IRosterIndex::IT_MyResourcesGroup:
  case IRosterIndex::IT_NotInRosterGroup:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(IRosterIndex::DR_Id);
    case Qt::BackgroundColorRole:
      return Qt::lightGray;
    case IRosterIndex::DR_ShowGroupExpander:
      return true;
    case IRosterIndex::DR_FontWeight:
      return QFont::DemiBold;
    } 
    break;
  
  case IRosterIndex::IT_Contact:
    switch (ARole)
    {
    case Qt::DisplayRole: 
      {
        Jid indexJid(AIndex->data(IRosterIndex::DR_Jid).toString());
        QString display = AIndex->data(IRosterIndex::DR_RosterName).toString();
        if (display.isEmpty())
          display = indexJid.bare();
        if (!indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    
    case Qt::DecorationRole: 
      return statusIcon(AIndex);
    } 
    break;
  
  case IRosterIndex::IT_Transport:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        QString display = AIndex->data(IRosterIndex::DR_RosterName).toString();
        if (display.isEmpty())
        {
          Jid indexJid(AIndex->data(IRosterIndex::DR_Jid).toString());
          display = indexJid.bare();
        }
        return display;
      }
    case Qt::DecorationRole: 
      return statusIcon(AIndex);
    } 
    break;
   
  case IRosterIndex::IT_MyResource:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        Jid indexJid(AIndex->data(IRosterIndex::DR_Jid).toString());
        return indexJid.resource();
      }
    case Qt::DecorationRole: 
      return statusIcon(AIndex);
    } 
    break;
 }
 return QVariant();
}

QList<int> IndexDataHolder::roles() const
{
  return QList<int>() << Qt::DisplayRole 
                      << Qt::DecorationRole 
                      << Qt::BackgroundColorRole 
                      << Qt::ForegroundRole
                      << IRosterIndex::DR_FontWeight
                      << IRosterIndex::DR_ShowGroupExpander;
}

QIcon IndexDataHolder::statusIcon(const IRosterIndex *AIndex) const
{
  if (!FStatusIconset.isValid())
    return QIcon();

  if (AIndex->data(IRosterIndex::DR_Ask).toString() == "subscribe")
    return FStatusIconset.iconByName("ask");
  if (AIndex->data(IRosterIndex::DR_Subscription).toString() == "none")
    return FStatusIconset.iconByName("noauth");

  switch (AIndex->data(IRosterIndex::DR_Show).toInt())
  {
  case IPresence::Offline: 
    return FStatusIconset.iconByName("offline");
  case IPresence::Online: 
    return FStatusIconset.iconByName("online");
  case IPresence::Chat: 
    return FStatusIconset.iconByName("chat");
  case IPresence::Away: 
    return FStatusIconset.iconByName("away");
  case IPresence::ExtendedAway: 
    return FStatusIconset.iconByName("xa");
  case IPresence::DoNotDistrib: 
    return FStatusIconset.iconByName("dnd");
  case IPresence::Error: 
    return FStatusIconset.iconByName("error");
  }
  return QIcon();
}
