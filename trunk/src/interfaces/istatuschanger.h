#ifndef ISTATUSCHANGER_H
#define ISTATUSCHANGER_H

#include "../../utils/jid.h"
#include "../../utils/menu.h"

#define STATUSCHANGER_UUID "{F0D57BD2-0CD4-4606-9CEE-15977423F8DC}"

#define ACTION_DR_STATUS_CODE               Action::DR_Parametr1

#define NULL_STATUS_ID                      0
#define STATUS_ERROR                        -2
#define MAIN_STATUS_ID                      -1

#define STATUS_ONLINE                       10
#define STATUS_CHAT                         15
#define STATUS_AWAY                         20
#define STATUS_EXAWAY                       25
#define STATUS_DND                          30
#define STATUS_INVISIBLE                    35
#define STATUS_OFFLINE                      40
#define MAX_STANDART_STATUS_ID              100

class IStatusChanger {
public:
  virtual QObject *instance() =0;
  virtual int mainStatus() const =0;
  virtual void setStatus(int AStatusId, const Jid &AStreamJid = Jid()) =0;
  virtual int streamStatus(const Jid &AStreamJid) const =0;
  virtual Menu *statusMenu() const =0;
  virtual Menu *streamMenu(const Jid &AStreamJid) const =0;
  virtual int addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority) =0;
  virtual void updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority) =0;
  virtual void removeStatusItem(int AStatusId) =0;
  virtual QString statusItemName(int AStatusId) const =0;
  virtual int statusItemShow(int AStatusId) const =0;
  virtual QString statusItemText(int AStatusId) const =0;
  virtual int statusItemPriority(int AStatusId) const =0;
  virtual QIcon statusItemIcon(int AStatusId) const =0;
  virtual void setStatusItemIcon(int AStatusId, const QIcon &AIcon) =0;
  virtual void setStatusItemIcon(int AStatusId, const QString &AIconsetFile, const QString &AIconName) =0;
  virtual QList<int> statusItems() const =0;
  virtual QList<int> activeStatusItems() const =0;
  virtual int statusByName(const QString &AName) const =0;
  virtual QList<int> statusByShow(int AShow) const =0;
  virtual QIcon iconByShow(int AShow) const =0;
  virtual QString nameByShow(int AShow) const =0;
signals:
  virtual void statusItemAdded(int AStatusId) =0;
  virtual void statusItemChanged(int AStatusId) =0;
  virtual void statusItemRemoved(int AStatusId) =0;
  virtual void statusAboutToBeSeted(int AStatusId, const Jid &AStreamJid) =0;
  virtual void statusSeted(int AStatusId, const Jid &AStreamJid) =0;
};

Q_DECLARE_INTERFACE(IStatusChanger,"Vacuum.Plugin.IStatusChanger/1.0")

#endif
