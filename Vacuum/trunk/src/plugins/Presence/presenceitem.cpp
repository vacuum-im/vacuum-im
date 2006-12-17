#include "presenceitem.h"

PresenceItem::PresenceItem(const Jid &AItemJid, QObject *parent)
  : QObject(parent)
{
  FItemJid = AItemJid;
  FPresence = qobject_cast<IPresence *>(parent);
}

PresenceItem::~PresenceItem()
{

}
