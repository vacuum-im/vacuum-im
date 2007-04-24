#ifndef ISTATUSCHANGER_H
#define ISTATUSCHANGER_H

#include "../../interfaces/ipresence.h"
#include "../../utils/menu.h"

#define STATUSCHANGER_UUID "{F0D57BD2-0CD4-4606-9CEE-15977423F8DC}"

class IStatusChanger {
public:
  virtual QObject *instance() =0;
  virtual Menu *baseMenu() const =0;
  virtual Menu *streamMenu(const Jid &AStreamJid) const =0;
  virtual IPresence::Show baseShow() const=0;
  virtual void setPresence(IPresence::Show AShow, const QString &AStatus, 
    int APriority, const Jid &AStreamJid = Jid()) =0;
public slots:
  virtual void onChangeStatusAction(bool) =0;
};

Q_DECLARE_INTERFACE(IStatusChanger,"Vacuum.Plugin.IStatusChanger/1.0")

#endif