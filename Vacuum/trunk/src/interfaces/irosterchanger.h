#ifndef IROSTERCHANGER_H
#define IROSTERCHANGER_H

#include "../../utils/jid.h"
#include "../../utils/menu.h"

#define ROSTERCHANGER_UUID "{018E7891-2743-4155-8A70-EAB430573500}"

class IRosterChanger
{
public:
  virtual void showAddContactDialog(const Jid &AStreamJid, const Jid &AJid, const QString &ANick,
    const QString &AGroup, const QString &ARequest) const =0;
  virtual Menu *addContactMenu() const =0;
};

Q_DECLARE_INTERFACE(IRosterChanger,"Vacuum.Plugin.IRosterChanger/1.0")

#endif