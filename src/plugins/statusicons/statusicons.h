#ifndef STATUSICONS_H
#define STATUSICONS_H

#include <QRegExp>
#include <QPointer>
#include "../../definations/resources.h"
#include "../../definations/statusicons.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionnodeorders.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/actiongroups.h"
#include "../../definations/menuicons.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/isettings.h"
#include "rosterindexdataholder.h"
#include "iconsoptionswidget.h"

class StatusIcons : 
  public QObject,
  public IPlugin,
  public IStatusIcons,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStatusIcons IOptionsHolder);
public:
  StatusIcons();
  ~StatusIcons();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return STATUSICONS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IStatusIcons
  virtual QString defaultSubStorage() const;
  virtual void setDefaultSubStorage(const QString &ASubStorage);
  virtual void insertRule(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
  virtual QStringList rules(RuleType ARuleType) const;
  virtual QString ruleSubStorage(const QString &APattern, RuleType ARuleType) const;
  virtual void removeRule(const QString &APattern, RuleType ARuleType);
  virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QIcon iconByJidStatus(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QString subStorageByJid(const Jid &AContactJid) const;
  virtual QString iconKeyByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
signals:
  virtual void defaultStorageChanged(const QString &ASubStorage);
  virtual void ruleInserted(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
  virtual void ruleRemoved(const QString &APattern, RuleType ARuleType);
  virtual void defaultIconsChanged();
  virtual void statusIconsChanged();
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void loadStorages();
  void clearStorages();
  void startStatusIconsChanged();
protected slots:
  void onStatusIconsChangedTimer();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsAccepted();
  void onOptionsRejected();
  void onDefaultStorageChanged();
  void onSetCustomIconset(bool);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  Menu *FCustomIconMenu;
  Action *FDefaultIconAction;
  QHash<QString,Action *> FCustomIconActions;
  IconStorage *FDefaultStorage;
  RosterIndexDataHolder *FDataHolder;
  QPointer<IconsOptionsWidget> FIconsOptionWidget;
private:
  bool FStatusIconsChangedStarted;
  QString FDefaultSubStorage;
  QSet<QString> FStatusRules;
  QHash<QString, QString> FUserRules;
  QHash<QString, QString> FDefaultRules;
  QHash<QString, IconStorage *> FStorages;
  mutable QHash<Jid, QString> FJid2Storage;
};

#endif // STATUSICONS_H
