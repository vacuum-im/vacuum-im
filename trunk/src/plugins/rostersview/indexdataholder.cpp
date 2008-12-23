#include "indexdataholder.h"

IndexDataHolder::IndexDataHolder(QObject *AParent) : QObject(AParent)
{
  FOptions = 0;
}

IndexDataHolder::~IndexDataHolder()
{

}

QList<int> IndexDataHolder::roles() const
{
  static QList<int> dataRoles  = QList<int>() 
    << Qt::DisplayRole 
    << Qt::BackgroundColorRole 
    << Qt::ForegroundRole
    << RDR_FontWeight;
  return dataRoles;
}

QList<int> IndexDataHolder::types() const
{
  static QList<int> indexTypes  = QList<int>()  
    << RIT_StreamRoot 
    << RIT_Group
    << RIT_BlankGroup
    << RIT_AgentsGroup
    << RIT_MyResourcesGroup
    << RIT_NotInRosterGroup
    << RIT_Contact
    << RIT_Agent
    << RIT_MyResource;
  return indexTypes;
}

QVariant IndexDataHolder::data(const IRosterIndex *AIndex, int ARole) const
{
  switch (AIndex->type())
  {
  case RIT_StreamRoot:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      {
        QString display = AIndex->data(RDR_Name).toString();
        if (display.isEmpty())
          display = AIndex->data(RDR_Jid).toString();
        return display;
      }
    case Qt::ForegroundRole:
      return Qt::white;
    case Qt::BackgroundColorRole:
      return Qt::gray;
    case RDR_FontWeight:
      return QFont::Bold;
    } 
    break;
  
  case RIT_Group:
  case RIT_BlankGroup:
  case RIT_AgentsGroup:
  case RIT_MyResourcesGroup:
  case RIT_NotInRosterGroup:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(RDR_Id);
    case Qt::ForegroundRole:
      return Qt::blue;
    case RDR_FontWeight:
      return QFont::DemiBold;
    } 
    break;
  
  case RIT_Contact:
    switch (ARole)
    {
    case Qt::DisplayRole: 
      {
        Jid indexJid(AIndex->data(RDR_Jid).toString());
        QString display = AIndex->data(RDR_Name).toString();
        if (display.isEmpty())
          display = indexJid.hBare();
        if (checkOption(IRostersView::ShowResource) && !indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    } 
    break;
  
  case RIT_Agent:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        QString display = AIndex->data(RDR_Name).toString();
        if (display.isEmpty())
        {
          Jid indexJid(AIndex->data(RDR_Jid).toString());
          display = indexJid.hBare();
        }
        return display;
      }
    } 
    break;
   
  case RIT_MyResource:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        Jid indexJid(AIndex->data(RDR_Jid).toString());
        return indexJid.resource();
      }
    } 
    break;
 }
 return QVariant();
}

bool IndexDataHolder::checkOption(IRostersView::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void IndexDataHolder::setOption(IRostersView::Option AOption, bool AValue)
{
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
  if (AOption == IRostersView::ShowResource || AOption == IRostersView::ShowStatusText)
    emit dataChanged(NULL,Qt::DisplayRole);
}

