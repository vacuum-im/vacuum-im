#include "indexdataholder.h"

IndexDataHolder::IndexDataHolder(QObject *AParent) :
  QObject(AParent)
{
  FStatusIconset.openFile(STATUS_ICONSETFILE);
  FOptions = 0;
}

IndexDataHolder::~IndexDataHolder()
{

}

QList<int> IndexDataHolder::roles() const
{
  static QList<int> dataRoles  = QList<int>() << Qt::DisplayRole 
    << Qt::BackgroundColorRole 
    << Qt::ForegroundRole
    << Qt::ToolTipRole
    << RDR_FontWeight;
  return dataRoles;
}

QList<int> IndexDataHolder::types() const
{
  static QList<int> indexTypes  = QList<int>()  << RIT_StreamRoot 
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
      return Qt::darkGray;
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
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
          display = indexJid.bare();
        if (checkOption(IRostersView::ShowResource) && !indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
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
          display = indexJid.bare();
        }
        return display;
      }
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
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
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
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
}


QString IndexDataHolder::toolTipText(const IRosterIndex *AIndex) const
{
  QString toolTip;
  QString mask = "<b>%1:</b> %2<br>";
  QString val = AIndex->data(RDR_Name).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Name")).arg(val));

  val = AIndex->data(RDR_Jid).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Jid")).arg(val));

  val = AIndex->data(RDR_Status).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Status")).arg(val));

  val = AIndex->data(RDR_Priority).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Priority")).arg(val));

  val = AIndex->data(RDR_Subscription).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Subscription")).arg(val));

  val = AIndex->data(RDR_Ask).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Ask")).arg(val));

  toolTip.chop(4);
  return toolTip;
}

