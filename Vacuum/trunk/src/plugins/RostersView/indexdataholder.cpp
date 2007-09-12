#include "indexdataholder.h"

IndexDataHolder::IndexDataHolder(QObject *AParent) :
  QObject(AParent)
{
  FStatusIconset.openFile(STATUS_ICONSETFILE);
}

IndexDataHolder::~IndexDataHolder()
{

}

bool IndexDataHolder::setData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
  QHash<int,QVariant> &values = FData[AIndex];
  if (AValue.isValid())
  {
    QVariant oldValue = values.value(ARole,AValue);
    values.insert(ARole,AValue);
    if (oldValue != AValue)
      emit dataChanged(AIndex,ARole);
  }
  else
  {
    QVariant oldValue = values.value(ARole,AValue);
    values.remove(ARole);
    if (values.isEmpty())
      FData.remove(AIndex);
    if (oldValue != AValue)
      emit dataChanged(AIndex,ARole);
  }
  return true;     
}

QVariant IndexDataHolder::data(const IRosterIndex *AIndex, int ARole) const
{
  QVariant result = FData.value(AIndex).value(ARole);
  if (result.isValid())
    return result;

  switch (AIndex->type())
  {
  case IRosterIndex::IT_StreamRoot:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      {
        QString display = AIndex->data(IRosterIndex::DR_Name).toString();
        if (display.isEmpty())
          display = AIndex->data(IRosterIndex::DR_Jid).toString();
        return display;
      }
    case Qt::ForegroundRole:
      return Qt::white;
    case Qt::BackgroundColorRole:
      return Qt::darkGray;
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
    case IRosterIndex::DR_FontWeight:
      return QFont::Bold;
    } 
    break;
  
  case IRosterIndex::IT_Group:
  case IRosterIndex::IT_BlankGroup:
  case IRosterIndex::IT_AgentsGroup:
  case IRosterIndex::IT_MyResourcesGroup:
  case IRosterIndex::IT_NotInRosterGroup:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(IRosterIndex::DR_Id);
    case Qt::ForegroundRole:
      return Qt::blue;
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
        QString display = AIndex->data(IRosterIndex::DR_Name).toString();
        if (display.isEmpty())
          display = indexJid.bare();
        if (!indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
    } 
    break;
  
  case IRosterIndex::IT_Agent:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        QString display = AIndex->data(IRosterIndex::DR_Name).toString();
        if (display.isEmpty())
        {
          Jid indexJid(AIndex->data(IRosterIndex::DR_Jid).toString());
          display = indexJid.bare();
        }
        return display;
      }
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
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
    case Qt::ToolTipRole:
      return toolTipText(AIndex);
    } 
    break;
 }
 return QVariant();
}

QList<int> IndexDataHolder::roles() const
{
  static QList<int> dataRoles  = QList<int>() << Qt::DisplayRole 
                                              << Qt::BackgroundColorRole 
                                              << Qt::ForegroundRole
                                              << Qt::ToolTipRole
                                              << IRosterIndex::DR_FontWeight
                                              << IRosterIndex::DR_ShowGroupExpander;
  return dataRoles;
}

void IndexDataHolder::clear()
{
  FData.clear();
}

QString IndexDataHolder::toolTipText(const IRosterIndex *AIndex) const
{
  QString toolTip;
  QString mask = "<b>%1:</b> %2<br>";
  QString val = AIndex->data(IRosterIndex::DR_Name).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Name")).arg(val));

  val = AIndex->data(IRosterIndex::DR_Jid).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Jid")).arg(val));

  val = AIndex->data(IRosterIndex::DR_Status).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Status")).arg(val));

  val = AIndex->data(IRosterIndex::DR_Priority).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Priority")).arg(val));

  val = AIndex->data(IRosterIndex::DR_Subscription).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Subscription")).arg(val));

  val = AIndex->data(IRosterIndex::DR_Ask).toString();
  if (!val.isEmpty())
    toolTip.append(mask.arg(tr("Ask")).arg(val));

  toolTip.chop(4);
  return toolTip;
}
