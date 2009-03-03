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
    << RDR_FONT_WEIGHT;
  return dataRoles;
}

QList<int> IndexDataHolder::types() const
{
  static QList<int> indexTypes  = QList<int>()  
    << RIT_STREAM_ROOT 
    << RIT_GROUP
    << RIT_GROUP_BLANK
    << RIT_GROUP_AGENTS
    << RIT_GROUP_MY_RESOURCES
    << RIT_GROUP_NOT_IN_ROSTER
    << RIT_CONTACT
    << RIT_AGENT
    << RIT_MY_RESOURCE;
  return indexTypes;
}

QVariant IndexDataHolder::data(const IRosterIndex *AIndex, int ARole) const
{
  switch (AIndex->type())
  {
  case RIT_STREAM_ROOT:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      {
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
          display = AIndex->data(RDR_JID).toString();
        return display;
      }
    case Qt::ForegroundRole:
      return Qt::white;
    case Qt::BackgroundColorRole:
      return Qt::gray;
    case RDR_FONT_WEIGHT:
      return QFont::Bold;
    } 
    break;
  
  case RIT_GROUP:
  case RIT_GROUP_BLANK:
  case RIT_GROUP_AGENTS:
  case RIT_GROUP_MY_RESOURCES:
  case RIT_GROUP_NOT_IN_ROSTER:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(RDR_INDEX_ID);
    case Qt::ForegroundRole:
      return Qt::blue;
    case RDR_FONT_WEIGHT:
      return QFont::DemiBold;
    } 
    break;
  
  case RIT_CONTACT:
    switch (ARole)
    {
    case Qt::DisplayRole: 
      {
        Jid indexJid(AIndex->data(RDR_JID).toString());
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
          display = indexJid.hBare();
        if (checkOption(IRostersView::ShowResource) && !indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    } 
    break;
  
  case RIT_AGENT:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
        {
          Jid indexJid(AIndex->data(RDR_JID).toString());
          display = indexJid.hBare();
        }
        return display;
      }
    } 
    break;
   
  case RIT_MY_RESOURCE:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        Jid indexJid(AIndex->data(RDR_JID).toString());
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

