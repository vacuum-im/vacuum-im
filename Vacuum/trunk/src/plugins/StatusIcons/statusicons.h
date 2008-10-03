#ifndef STATUSICONS_H
#define STATUSICONS_H

#include <QRegExp>
#include <QPointer>
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iskinmanager.h"
#include "../../utils/skin.h"
#include "../../utils/unzipfile.h"
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
  virtual QString defaultIconFile() const { return FDefaultIconFile; }
  virtual void setDefaultIconFile(const QString &AIconFile);
  virtual void insertRule(const QString &APattern, const QString &AIconFile, RuleType ARuleType);
  virtual QStringList rules(RuleType ARuleType) const;
  virtual QString ruleIconFile(const QString &APattern, RuleType ARuleType) const;
  virtual void removeRule(const QString &APattern, RuleType ARuleType);
  virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AJid) const;
  virtual QIcon iconByJidStatus(const Jid &AJid, int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QString iconFileByJid(const Jid &AJid) const;
  virtual QString iconNameByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
signals:
  virtual void defaultIconFileChanged(const QString &AIconFile);
  virtual void ruleInserted(const QString &APattern, const QString &AIconFile, RuleType ARuleType);
  virtual void ruleRemoved(const QString &APattern, RuleType ARuleType);
  virtual void defaultIconsChanged();
  virtual void statusIconsChanged();
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void startStatusIconsChanged();
  void loadIconFilesRules();
  void clearIconFilesRules();
protected slots:
  void onStatusIconsChangedTimer();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsAccepted();
  void onOptionsRejected();
  void onStatusIconsetChanged();
  void onSetCustomIconset(bool);
  void onSkinChanged();
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
  ISkinManager *FSkinManager;
private:
  Menu *FCustomIconMenu;
  Action *FDefaultIconAction;
  QHash<QString,Action *> FCustomIconActions;
  SkinIconset *FStatusIconset;
  RosterIndexDataHolder *FDataHolder;
  QPointer<IconsOptionsWidget> FIconsOptionWidget;
private:
  bool FStatusIconsChangedStarted;
  QString FDefaultIconFile;
  QHash<QString, QString> FUserRules;
  QHash<QString, QString> FDefaultRules;
  QSet<QString> FIconFilesRules;
  mutable QHash<QString, QString> FJid2IconFile;
};

#endif // STATUSICONS_H
