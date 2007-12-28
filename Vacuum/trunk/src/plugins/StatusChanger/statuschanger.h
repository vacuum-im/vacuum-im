#ifndef STATUSCHANGER_H
#define STATUSCHANGER_H

#include <QSet>
#include <QHash>
#include <QPair>
#include <QDateTime>
#include <QPointer>
#include "../../definations/actiongroups.h"
#include "../../definations/initorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatuschanger.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/istatusicons.h"
#include "../../utils/skin.h"
#include "editstatusdialog.h"
#include "accountoptionswidget.h"

struct StatusItem {
  int code;
  QString name;
  int show;
  QString text;
  int priority;
  QIcon icon;
  QString iconsetFile;
  QString iconName;
};

class StatusChanger : 
  public QObject,
  public IPlugin,
  public IStatusChanger,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStatusChanger IOptionsHolder);

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
  virtual bool startPlugin();
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IStatusChanger
  virtual int mainStatus() const { return FStatusItems.value(-1)->code; }
  virtual void setStatus(int AStatusId, const Jid &AStreamJid = Jid());
  virtual int streamStatus(const Jid &AStreamJid) const;
  virtual Menu *statusMenu() const { return FMainMenu; }
  virtual Menu *streamMenu(const Jid &AStreamJid) const;
  virtual int addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority);
  virtual void updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority);
  virtual void removeStatusItem(int AStatusId);
  virtual QString statusItemName(int AStatusId) const;
  virtual int statusItemShow(int AStatusId) const;
  virtual QString statusItemText(int AStatusId) const;
  virtual int statusItemPriority(int AStatusId) const;
  virtual QIcon statusItemIcon(int AStatusId) const;
  virtual void setStatusItemIcon(int AStatusId, const QIcon &AIcon);
  virtual void setStatusItemIcon(int AStatusId, const QString &AIconsetFile, const QString &AIconName);
  virtual QList<int> statusItems() const { return FStatusItems.keys(); }
  virtual QList<int> activeStatusItems() const;
  virtual int statusByName(const QString &AName) const;
  virtual QList<int> statusByShow(int AShow) const;
  virtual QIcon iconByShow(int AShow) const;
  virtual QString nameByShow(int AShow) const;
public slots:
  virtual void setStatusByAction(bool);
signals:
  virtual void statusItemAdded(int AStatusId);
  virtual void statusItemChanged(int AStatusId);
  virtual void statusItemRemoved(int AStatusId);
  virtual void statusAboutToBeSeted(int AStatusId, const Jid &AStreamJid);
  virtual void statusSeted(int AStatusId, const Jid &AStreamJid);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void createDefaultStatus();
  void setMainStatusId(int AStatusId);
  void setStreamStatusId(IPresence *APresence, int AStatusId);
  Action *createStatusAction(int AStatusId, const Jid &AStreamJid, QObject *AParent) const;
  void updateStatusAction(StatusItem *AStatus, Action *AAction) const;
  void createStatusActions(int AStatusId);
  void updateStatusActions(int AStatusId);
  void removeStatusActions(int AStatusId);
  void updateCustomMenu();
  void createStreamMenu(IPresence *APresence);
  void updateStreamMenu(IPresence *APresence);
  void removeStreamMenu(IPresence *APresence);
  void updateMainMenu();
  void updateTrayToolTip();
  void updateMainStatusActions();
  void insertConnectingLabel(IPresence *APresence);
  void removeConnectingLabel(IPresence *APresence);
  void autoReconnect(IPresence *APresence);
  int createTempStatus(IPresence *APresence, int AShow, const QString &AText, int APriority);
  void removeTempStatus(IPresence *APresence);
  void resendUpdatedStatus(int AStatusId);
  void removeAllCustomStatuses();
protected slots:
  void onPresenceAdded(IPresence *APresence);
  void onSelfPresence(IPresence *, IPresence::Show AShow, 
    const QString &AStatus, qint8 APriority, const Jid &AJid);
  void onPresenceRemoved(IPresence *APresence);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onAccountShown(IAccount *AAccount);
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onDefaultStatusIconsChanged();
  void onRosterIconsetChanged();
  void onSettingsOpened();
  void onSettingsClosed();
  void onReconnectTimer();
  void onEditStatusAction(bool);
  void onOptionsAccepted();
  void onOptionsRejected();
  void onOptionsDialogClosed();
  void onAccountChanged(const QString &AName, const QVariant &AValue);
private:
  IPresencePlugin *FPresencePlugin;
  IRosterPlugin *FRosterPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersModel *FRostersModel;
  IRostersModelPlugin *FRostersModelPlugin;
  ISettingsPlugin *FSettingsPlugin;
  ITrayManager *FTrayManager;
  IAccountManager *FAccountManager;
  IStatusIcons *FStatusIcons;
private:
  int FConnectingLabel;
  SkinIconset *FRosterIconset;
private:
  Action *FEditStatusAction;
  QPointer<EditStatusDialog> FEditStatusDialog;
private:
  QToolButton *FMainMenuToolButton;
  Menu *FMainMenu;
  Menu *FCustomMenu;
  QHash<IPresence *, Menu *> FStreamMenu;
  QHash<IPresence *, Menu *> FStreamCustomMenu;
  QHash<IPresence *, Action *> FStreamMainStatusAction;
private:
  IPresence *FSettingStatusToPresence;
  QHash<int, StatusItem *> FStatusItems;
  QHash<IPresence *, int> FStreamStatus;
  QHash<IPresence *, int> FStreamWaitStatus;
  QHash<Jid, int> FStreamLastStatus;
  QHash<IPresence *,QPair<QDateTime,int> >FStreamWaitReconnect;
  QHash<IPresence *,int> FStreamTempStatus;
  QSet<IPresence *> FStreamMainStatus;
  QHash<QString,AccountOptionsWidget *> FAccountOptionsById;
};

#endif // STATUSCHANGER_H
