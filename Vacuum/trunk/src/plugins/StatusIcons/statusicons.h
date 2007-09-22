#ifndef STATUSICONS_H
#define STATUSICONS_H

#include <QRegExp>
#include <QPointer>
#include "../../definations/initorders.h"
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

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return STATUSICONS_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
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
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void repaintRostersView();
  void loadIconFilesRules();
  void clearIconFilesRules();
protected slots:
  void onRepaintRostersView();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsAccepted();
  void onOptionsRejected();
  void onStatusIconsetChanged();
  void onSetCustomIconset(bool);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IRostersModelPlugin *FRostersModelPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  Menu *FCustomIconMenu;
  Action *FDefaultIconAction;
  QHash<QString,Action *> FCustomIconActions;
  SkinIconset *FStatusIconset;
  RosterIndexDataHolder *FDataHolder;
  QPointer<IconsOptionsWidget> FIconsOptionWidget;
private:
  bool FRepaintStarted;
  QString FDefaultIconFile;
  QHash<QString, QString> FUserRules;
  QHash<QString, QString> FDefaultRules;
  QSet<QString> FIconFilesRules;
  mutable QHash<QString, QString> FJid2IconFile;
};

#endif // STATUSICONS_H
