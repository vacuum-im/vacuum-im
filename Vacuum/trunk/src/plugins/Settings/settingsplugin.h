#ifndef SETTINGSPLUGIN_H
#define SETTINGSPLUGIN_H

#include <QObject>
#include <QFile>
#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/isettings.h"
#include "settings.h"

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
	virtual QUuid getPluginUuid() const { return SETTINGS_UUID; }
	virtual void getPluginInfo(PluginInfo *);
	virtual bool initPlugin(IPluginManager *);
	virtual bool startPlugin();

  //ISettings
  virtual ISettings *newSettings(const QUuid &APluginId, QObject *AParent);
  virtual QString fileName() const { return FFile.fileName(); }
  virtual bool setFileName(const QString &AFileName);
  virtual bool saveSettings();
  virtual QDomDocument document() const { return FSettings; }
  virtual QString profile() const { return FProfile.tagName(); }
  virtual QDomElement setProfile(const QString &AProfile);
  virtual QDomElement getProfile(const QString &AProfile);
  virtual QDomElement getPluginNode(const QUuid &AId);
signals:
  virtual void profileOpened();
  virtual void profileClosed();
private slots:
  void onPluginManagerQuit();
private:
  IPluginManager *FPluginManager;
private:
  QFile FFile;
  QDomDocument FSettings;
  QDomElement FProfile;
  bool FProfileOpened;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // SETTINGSPLUGIN_H
