#ifndef DEFAULTCONNECTIONPLUGIN_H
#define DEFAULTCONNECTIONPLUGIN_H

#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/isettings.h>
#include "defaultconnection.h"
#include "connectionoptionswidget.h"

class DefaultConnectionPlugin : 
  public QObject,
  public IPlugin,
  public IConnectionPlugin,
  public IDefaultConnectionPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IConnectionPlugin IDefaultConnectionPlugin);
public:
  DefaultConnectionPlugin();
  ~DefaultConnectionPlugin();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return DEFAULTCONNECTION_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IConnectionPlugin
  virtual QString displayName() const;
  virtual IConnection *newConnection(const QString &ASettingsNS, QObject *AParent);
  virtual void destroyConnection(IConnection *AConnection);
  virtual QWidget *settingsWidget(const QString &ASettingsNS);
  virtual void saveSettings(QWidget *AWidget, const QString &ASettingsNS = QString::null);
  virtual void loadSettings(IConnection *AConnection, const QString &ASettingsNS);
  virtual void deleteSettings(const QString &ASettingsNS);
signals:
  void connectionCreated(IConnection *AConnection);
  void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS);
  void connectionDestroyed(IConnection *AConnection);
protected slots:
  void onConnectionAboutToConnect();
private:
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;  
  IXmppStreams *FXmppStreams;
  IConnectionManager *FConnectionManager;
private:
  QObjectCleanupHandler FCleanupHandler;
};

#endif // DEFAULTCONNECTIONPLUGIN_H
