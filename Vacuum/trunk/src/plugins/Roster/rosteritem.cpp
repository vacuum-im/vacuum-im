#include "rosteritem.h"

RosterItem::RosterItem(const Jid &AJid, IRoster *ARoster) : QObject(ARoster->instance())
{
  FJid = AJid; 
  FRoster = ARoster; 
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
