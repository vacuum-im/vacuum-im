#include "rosteritem.h"

RosterItem::RosterItem(const Jid &AJid, QObject *parent)
  : QObject(parent)
{
  FJid = AJid; 
  FRoster = qobject_cast<IRoster *>(parent); 
}

RosterItem::~RosterItem()
{

}

QString RosterItem::name() const
{
  if (FName.isEmpty()) 
    return FJid.bare();

  return FName; 
}
