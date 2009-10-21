#ifndef SETTINGSPLUGIN_H
#define SETTINGSPLUGIN_H

#include <QMap>
#include <QHash>
#include <QFile>
#include <QPointer>
#include <QWidget>
#include <definations/actiongroups.h>
#include <definations/commandline.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isettings.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <utils/action.h>
#include "settings.h"
#include "optionsdialog.h"
#include "profiledialog.h"
#include "miscoptionswidget.h"

class SettingsPlugin : 
  public QObject,
  public IPlugin,
  public ISettingsPlugin,
  public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISettingsPlugin IOptionsHolder);
public:
  SettingsPlugin();
  ~SettingsPlugin();
  virtual QObject *instance() {return this;}
  //IPlugin
  virtual QUuid pluginUuid() const { return SETTINGS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //ISettings
    //Profiles
  virtual bool isProfilesValid() const { return !FProfiles.isNull(); }
  virtual bool isProfileOpened() const { return FProfileOpened; }
  virtual const QDir &homeDir() const { return FHomeDir; }
  virtual const QDir &profileDir() const { return FProfileDir; }
  virtual bool addProfile(const QString &AProfile);
  virtual QString profile() const { return FProfile.attribute("name"); }
  virtual QStringList profiles() const;
  virtual QDomElement profileNode(const QString &AProfile);
  virtual QDomElement pluginNode(const QUuid &APluginId);
  virtual bool setProfile(const QString &AProfile);
  virtual bool renameProfile(const QString &AProfileFrom, const QString &AProfileTo);
  virtual bool removeProfile(const QString &AProfile);
    //Settings
  virtual bool saveSettings();
  virtual ISettings *settingsForPlugin(const QUuid &APluginId);
    //OptionsDialog
  virtual void insertOptionsHolder(IOptionsHolder *AOptionsHolder);
  virtual void removeOptionsHolder(IOptionsHolder *AOptionsHolder);
  virtual void openOptionsNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIconKey, int AOrder);
  virtual void closeOptionsNode(const QString &ANode);
  virtual QDialog *openOptionsDialog(const QString &ANode = "", QWidget *AParent = NULL);
signals:
  virtual void profileAdded(const QString &AProfile);
  virtual void settingsOpened();
  virtual void profileOpened(const QString &AProfile);
  virtual void profileClosed(const QString &AProfile);
  virtual void settingsClosed();
  virtual void profileRenamed(const QString &AProfileFrom, const QString &AProfileTo);
  virtual void profileRemoved(const QString &AProfile);
  virtual void optionsNodeOpened(const QString &ANode);
  virtual void optionsNodeClosed(const QString &ANode);
  virtual void optionsDialogOpened();
  virtual void optionsDialogAccepted();
  virtual void optionsDialogRejected();
  virtual void optionsDialogClosed();
signals:
  virtual void optionsAccepted();
  virtual void optionsRejected();
public:
  QWidget *createNodeWidget(const QString &ANode);
protected:
  void setProfileOpened();
  void setProfileClosed();
  void updateSettings();
  void addProfileAction(const QString &AProfile);
  void setActiveProfileAction(const QString &AProfile);
  void renameProfileAction(const QString &AProfileFrom, const QString &AProfileTo);
  void removeProfileAction(const QString &AProfile);
protected slots:
  void onOpenProfileDialogByAction(bool);
  void onOpenOptionsDialogByAction(bool);
  void onSetProfileByAction(bool);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onOptionsDialogClosed();
  void onPluginManagerQuit();
private:
  IPluginManager *FPluginManager;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
private:
  Action *FOpenOptionsDialogAction;
  Action *FOpenProfileDialogAction;
  Menu *FProfileMenu;
private:
  bool FProfileOpened;
  QDir FHomeDir;
  QDir FProfileDir;
  QDomElement FProfile;
  QDomDocument FProfiles;
  QDomDocument FSettings;
private:
  QMap<QString, OptionsNode *> FNodes;
  QHash<QUuid,Settings *> FPluginSettings;
  QList<IOptionsHolder *> FOptionsHolders;
  QPointer<OptionsDialog> FOptionsDialog;
  QPointer<ProfileDialog> FProfileDialog;
};

#endif // SETTINGSPLUGIN_H
