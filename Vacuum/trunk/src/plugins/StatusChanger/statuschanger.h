#ifndef STATUSCHANGER_H
#define STATUSCHANGER_H

#include <QHash>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatuschanger.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersview.h"
#include "../../utils/skin.h"

#define STATUSCHANGER_UUID "{F0D57BD2-0CD4-4606-9CEE-15977423F8DC}"

#define STATUSMENU_MENU_MAIN_ORDER 200
#define STATUSMENU_MENU_STREAM_ORDER 190

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
  virtual QUuid getPluginUuid() const { return STATUSCHANGER_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();
  
  //IStatusChanger
  virtual Menu *mainMenu() const { return FMenu; }
  virtual Menu *streamMenu(const Jid &AStreamJid) const;
  virtual IPresence::Show baseShow() const { return FBaseShow; }
  virtual void setPresence(IPresence::Show AShow, const QString &AStatus, 
    int APriority, const Jid &AStreamJid = Jid());
protected:
  void setBaseShow(IPresence::Show AShow);
  void createStatusActions(IPresence *APresence = NULL);
  void updateMenu(IPresence *APresence = NULL);
  void addStreamMenu(IPresence *APresence);
  void removeStreamMenu(IPresence *APresence);
  QIcon getStatusIcon(IPresence::Show AShow) const;
  QString getStatusName(IPresence::Show AShow) const;
  QString getStatusText(IPresence::Show AShow) const;
  int getStatusPriority(IPresence::Show AShow) const;
protected slots:
  void onStatusTriggered(bool);
  void onPresenceAdded(IPresence *APresence);
  void onPresenceOpened(IPresence *APresence);
  void onSelfPresence(IPresence *, IPresence::Show AShow, 
    const QString &, qint8 , const Jid &);
  void onPresenceClosed(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
  void onSkinChanged(const QString &ASkinName);
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
private:
  IPresencePlugin *FPresencePlugin;
  //IRosterPlugin *FRosterPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  Menu *FMenu;
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
  SkinIconset FStatusIconset;
};

#endif // STATUSCHANGER_H
