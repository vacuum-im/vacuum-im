#ifndef STATUSCHANGER_H
#define STATUSCHANGER_H

#include <QHash>
#include <QPair>
#include <QDateTime>
#include "../../definations/actiongroups.h"
#include "../../definations/initorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatuschanger.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../utils/skin.h"


class StatusChanger : 
  public QObject,
  public IPlugin,
  public IStatusChanger
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStatusChanger);

public:
  StatusChanger();
  ~StatusChanger();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return STATUSCHANGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  
  //IStatusChanger
  virtual Menu *baseMenu() const { return mnuBase; }
  virtual Menu *streamMenu(const Jid &AStreamJid) const;
  virtual IPresence::Show baseShow() const { return FBaseShow; }
  virtual void setPresence(IPresence::Show AShow, const QString &AStatus, 
    int APriority, const Jid &AStreamJid = Jid());
public slots:
  virtual void onChangeStatusAction(bool);
protected:
  struct PresenceItem {
    IPresence::Show show;
    QString status;
    int priority;
  };
  void setBaseShow(IPresence::Show AShow);
  void createStatusActions(IPresence *APresence = NULL);
  void updateMenu(IPresence *APresence = NULL);
  void addStreamMenu(IPresence *APresence);
  void removeStreamMenu(IPresence *APresence);
  void updateAccount(IPresence *APresence);
  void autoReconnect(IPresence *APresence);
  QString getStatusIconName(IPresence::Show AShow) const;
  QString getStatusName(IPresence::Show AShow) const;
  QString getStatusText(IPresence::Show AShow) const;
  int getStatusPriority(IPresence::Show AShow) const;
  void insertWaitOnline(IPresence *APresence, PresenceItem &AItem);
  void removeWaitOnline(IPresence *APresence);
protected slots:
  void onPresenceAdded(IPresence *APresence);
  void onSelfPresence(IPresence *, IPresence::Show AShow, 
    const QString &AStatus, qint8 APriority, const Jid &AJid);
  void onPresenceRemoved(IPresence *APresence);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
  void onReconnectTimer();
  void onAccountShown(IAccount *AAccount);
  void onSkinChanged();
private:
  IPresencePlugin *FPresencePlugin;
  IRosterPlugin *FRosterPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersModel *FRostersModel;
  IRostersModelPlugin *FRostersModelPlugin;
  IAccountManager *FAccountManager;
private:
  Menu *mnuBase;
  QHash<IPresence *, Menu *> FStreamMenus;
private:
  IPresence::Show FBaseShow;
  int FConnectingLabel;
  SkinIconset FRosterIconset;
  QList<IPresence *> FPresences;
  QHash<IPresence *,PresenceItem> FWaitOnline;
  QHash<IPresence *,IAccount *> FAccounts;
  QHash<IPresence *,QPair<QDateTime,PresenceItem> > FWaitReconnect;
};

#endif // STATUSCHANGER_H
