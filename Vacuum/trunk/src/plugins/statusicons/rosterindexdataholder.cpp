#include "rosterindexdataholder.h"

RosterIndexDataHolder::RosterIndexDataHolder(IStatusIcons *AStatusIcons)
  : QObject(AStatusIcons->instance())
{
  FStatusIcons = AStatusIcons;
  connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
}

RosterIndexDataHolder::~RosterIndexDataHolder()
{

}

QList<int> RosterIndexDataHolder::roles() const
{
  return QList<int>() << Qt::DecorationRole;
}

QList<int> RosterIndexDataHolder::types() const
{
  return QList<int>() << RIT_StreamRoot 
                      << RIT_Contact 
                      << RIT_Agent 
                      << RIT_MyResource;
}

QVariant RosterIndexDataHolder::data(const IRosterIndex *AIndex, int /*ARole*/) const
{
  Jid jid = AIndex->data(RDR_Jid).toString();
  if (jid.isValid())
  {
    int show = AIndex->data(RDR_Show).toInt();
    QString subs = AIndex->data(RDR_Subscription).toString();
    bool ask = !AIndex->data(RDR_Ask).toString().isEmpty();
    return FStatusIcons->iconByJidStatus(jid,show,subs,ask);
  }
  return QVariant();
}

void RosterIndexDataHolder::onStatusIconsChanged()
{
  emit dataChanged(NULL,Qt::DecorationRole);
}

