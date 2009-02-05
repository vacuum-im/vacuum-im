#include "rosterindexdataholder.h"

RosterIndexDataHolder::RosterIndexDataHolder(IStatusIcons *AStatusIcons) : QObject(AStatusIcons->instance())
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
  Jid contactJid = AIndex->data(RDR_Jid).toString();
  if (contactJid.isValid())
  {
    int show = AIndex->data(RDR_Show).toInt();
    QString subscription = AIndex->data(RDR_Subscription).toString();
    bool ask = !AIndex->data(RDR_Ask).toString().isEmpty();
    return FStatusIcons->iconByJidStatus(contactJid,show,subscription,ask);
  }
  return QVariant();
}

void RosterIndexDataHolder::onStatusIconsChanged()
{
  emit dataChanged(NULL,Qt::DecorationRole);
}

