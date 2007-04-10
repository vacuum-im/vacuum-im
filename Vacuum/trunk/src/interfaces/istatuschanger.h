#ifndef ISTATUSCHANGER_H
#define ISTATUSCHANGER_H

#include "../../interfaces/ipresence.h"
#include "../../utils/menu.h"

class IStatusChanger {
public:
  virtual QObject *instance() =0;
  virtual Menu *mainMenu() const =0;
  virtual Menu *streamMenu(const Jid &AStreamJid) const =0;
  virtual IPresence::Show baseShow() const=0;
  virtual void setPresence(IPresence::Show AShow, const QString &AStatus, 
    int APriority, const Jid &AStreamJid = Jid()) =0;
public slots:
  virtual void onChangeStatusAction(bool) =0;
};

Q_DECLARE_INTERFACE(IStatusChanger,"Vacuum.Plugin.IStatusChanger/1.0")

#endif