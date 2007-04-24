#ifndef STATUSCHANGER_H
#define STATUSCHANGER_H

#include <QHash>
#include <QPair>
#include <QDateTime>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatuschanger.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iaccountmanager.h"

#define STATUSCHANGER_UUID "{F0D57BD2-0CD4-4606-9CEE-15977423F8DC}"
#define STATUSCHANGER_INITORDER 100;

#define STATUSMENU_ACTION_GROUP_MAIN 200
#define STATUSMENU_ACTION_GROUP_STREAM 190

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
protected slots:
  void onRostersViewCreated(IRostersView *ARostersView);
  void onPresenceAdded(IPresence *APresence);
  void onSelfPresence(IPresence *, IPresence::Show AShow, 
    const QString &AStatus, qint8 APriority, const Jid &AJid);
  void onPresenceRemoved(IPresence *APresence);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
  void onReconnectTimer();
  void onAccountShown(IAccount *AAccount);
private:
  IPresencePlugin *FPresencePlugin;
  IRosterPlugin *FRosterPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
  IAccountManager *FAccountManager;
private:
  Menu *mnuBase;
  QHash<IPresence *, Menu *> FStreamMenus;
private:
  struct PresenceItem {
    IPresence::Show show;
    QString status;
    int priority;
  };
  IPresence::Show FBaseShow;
  QList<IPresence *> FPresences;
  QHash<IPresence *,PresenceItem> FWaitOnline;
  QHash<IPresence *,IAccount *> FAccounts;
  QHash<IPresence *,QPair<QDateTime,PresenceItem> > FWaitReconnect;
};

#endif // STATUSCHANGER_H
