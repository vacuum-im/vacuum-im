#include "rosterindexdataholder.h"

RosterIndexDataHolder::RosterIndexDataHolder(IStatusIcons *AStatusIcons)
  : QObject(AStatusIcons->instance())
{
  FStatusIcons = AStatusIcons;
}

RosterIndexDataHolder::~RosterIndexDataHolder()
{

}

QVariant RosterIndexDataHolder::data(const IRosterIndex *AIndex, int /*ARole*/) const
{
  Jid jid = AIndex->data(IRosterIndex::DR_Jid).toString();
  if (jid.isValid())
  {
    int show = AIndex->data(IRosterIndex::DR_Show).toInt();
    QString subs = AIndex->data(IRosterIndex::DR_Subscription).toString();
    bool ask = !AIndex->data(IRosterIndex::DR_Ask).toString().isEmpty();
    return FStatusIcons->iconByJidStatus(jid,show,subs,ask);
  }
  return QVariant();
}

bool RosterIndexDataHolder::setData( IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/ )
{
  return false;
}

QList<int> RosterIndexDataHolder::roles() const
{
  return QList<int>() << Qt::DecorationRole;
}

QList<int> RosterIndexDataHolder::types() const
{
  return QList<int>() << IRosterIndex::IT_StreamRoot 
                      << IRosterIndex::IT_Contact 
                      << IRosterIndex::IT_Agent 
                      << IRosterIndex::IT_MyResource;
}