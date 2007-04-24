#ifndef SETTINGSPLUGIN_H
#define SETTINGSPLUGIN_H

#include <QMap>
#include <QFile>
#include <QPointer>
#include <QObjectCleanupHandler>
#include <QWidget>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/imainwindow.h"
#include "../../utils/action.h"
#include "../../utils/skin.h"
#include "settings.h"
#include "optionsdialog.h"

#define SETTINGS_UUID "{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"

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
  virtual bool initObjects() { return true; }
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }

  //ISettings
  virtual ISettings *openSettings(const QUuid &APluginId, QObject *AParent);
  virtual QString settingsFile() const { return FFile.fileName(); }
  virtual bool setSettingsFile(const QString &AFileName);
  virtual bool saveSettings();
  virtual QDomDocument document() const { return FSettings; }
  virtual QString profile() const { return FProfile.tagName(); }
  virtual QDomElement setProfile(const QString &AProfile);
  virtual QDomElement profileNode(const QString &AProfile);
  virtual QDomElement pluginNode(const QUuid &AId);
  virtual void openOptionsDialog(const QString &ANode = "");
  virtual void openOptionsNode(const QString &ANode, const QString &AName, 
    const QString &ADescription, const QIcon &AIcon);
  virtual void closeOptionsNode(const QString &ANode);
  virtual void appendOptionsHolder(IOptionsHolder *AOptionsHolder);
  virtual void removeOptionsHolder(IOptionsHolder *AOptionsHolder);
public slots:
  virtual void openOptionsDialogAction(bool);
signals:
  virtual void profileOpened();
  virtual void profileClosed();
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
protected slots:
  void onMainWindowCreated(IMainWindow *AMainWindow);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onPluginManagerQuit();
private:
  IPluginManager *FPluginManager;
  IMainWindowPlugin *FMainWindowPlugin;
private:
  SkinIconset FSystemIconset;
  Action *actOpenOptionsDialog;
private:
  QFile FFile;
  QDomDocument FSettings;
  QDomElement FProfile;
  bool FProfileOpened;
  QObjectCleanupHandler FCleanupHandler;
private:
  struct OptionsNode  
  {
    QString name;
    QString desc;
    QIcon icon;
  };
  QMap<QString, OptionsNode *> FNodes;
  QList<IOptionsHolder *> FOptionsHolders;
  QPointer<OptionsDialog> FOptionsDialog;
};

#endif // SETTINGSPLUGIN_H
