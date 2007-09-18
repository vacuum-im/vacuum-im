#ifndef SETTINGSPLUGIN_H
#define SETTINGSPLUGIN_H

#include <QMap>
#include <QHash>
#include <QFile>
#include <QPointer>
#include <QWidget>
#include "../../definations/actiongroups.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../utils/action.h"
#include "../../utils/skin.h"
#include "settings.h"
#include "optionsdialog.h"
#include "profiledialog.h"

class SettingsPlugin : 
  public QObject,
  public IPlugin,
  public ISettingsPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISettingsPlugin);

public:
  SettingsPlugin();
  ~SettingsPlugin();
  virtual QObject *instance() {return this;}

  //IPlugin
  virtual QUuid pluginUuid() const { return SETTINGS_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }

  //ISettings
  virtual bool isProfilesValid() const { return !FProfiles.isNull(); }
  virtual bool isProfileOpened() const { return FProfileOpened; }
  virtual const QDir &homeDir() const { return FHomeDir; }
  virtual const QDir &profileDir() const { return FProfileDir; }
  virtual ISettings *settingsForPlugin(const QUuid &APluginId);
  virtual bool saveSettings();
  virtual bool addProfile(const QString &AProfile);
  virtual QString profile() const { return FProfile.attribute("name"); }
  virtual QStringList profiles() const;
  virtual QDomElement profileNode(const QString &AProfile);
  virtual QDomElement pluginNode(const QUuid &APluginId);
  virtual bool setProfile(const QString &AProfile);
  virtual bool renameProfile(const QString &AProfileFrom, const QString &AProfileTo);
  virtual bool removeProfile(const QString &AProfile);
  virtual void openOptionsDialog(const QString &ANode = "");
  virtual void openOptionsNode(const QString &ANode, const QString &AName, 
    const QString &ADescription, const QIcon &AIcon);
  virtual void closeOptionsNode(const QString &ANode);
  virtual void appendOptionsHolder(IOptionsHolder *AOptionsHolder);
  virtual void removeOptionsHolder(IOptionsHolder *AOptionsHolder);
public slots:
  virtual void openOptionsDialogByAction(bool);
  virtual void openProfileDialogByAction(bool);
signals:
  virtual void profileAdded(const QString &AProfile);
  virtual void profileOpened(const QString &AProfile);
  virtual void profileClosed(const QString &AProfile);
  virtual void profileRenamed(const QString &AProfileFrom, const QString &AProfileTo);
  virtual void profileRemoved(const QString &AProfile);
  virtual void settingsOpened();
  virtual void settingsClosed();
  virtual void optionsNodeOpened(const QString &ANode);
  virtual void optionsNodeClosed(const QString &ANode);
  virtual void optionsHolderAdded(IOptionsHolder *);
  virtual void optionsHolderRemoved(IOptionsHolder *);
  virtual void optionsDialogOpened();
  virtual void optionsDialogAccepted();
  virtual void optionsDialogRejected();
  virtual void optionsDialogClosed();
protected:
  QWidget *createNodeWidget(const QString &ANode);
  void setProfileOpened();
  void setProfileClosed();
  void updateSettings();
protected slots:
  void onMainWindowCreated(IMainWindow *AMainWindow);
  void onOptionsDialogOpened();
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onOptionsDialogClosed();
  void onPluginManagerQuit();
  void onSystemIconsetChanged();
private:
  IPluginManager *FPluginManager;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
private:
  SkinIconset *FSystemIconset;
  Action *FOpenOptionsDialogAction;
  Action *FOpenProfileDialogAction;
private:
  bool FProfileOpened;
  QDir FHomeDir;
  QDir FProfileDir;
  QDomElement FProfile;
  QDomDocument FProfiles;
  QDomDocument FSettings;
private:
  QHash<QUuid,Settings *> FPluginSettings;
  struct OptionsNode  
  {
    QString name;
    QString desc;
    QIcon icon;
  };
  QMap<QString, OptionsNode *> FNodes;
  QList<IOptionsHolder *> FOptionsHolders;
  QPointer<OptionsDialog> FOptionsDialog;
  QPointer<ProfileDialog> FProfileDialog;
};

#endif // SETTINGSPLUGIN_H
