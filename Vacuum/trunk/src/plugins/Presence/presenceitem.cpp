#include "presenceitem.h"

PresenceItem::PresenceItem(const Jid &AItemJid, QObject *AParent) : QObject(AParent)
{
  FItemJid = AItemJid;
  FPresence = qobject_cast<IPresence *>(AParent);
}

PresenceItem::~PresenceItem()
{

}
