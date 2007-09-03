#ifndef CONNECTIONMANAGER_H 
#define CONNECTIONMANAGER_H 

#include <QComboBox>
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iconnectionmanager.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/idefaultconnection.h"
#include "connectionoptionswidget.h"

class ConnectionManager :
  public QObject,
  public IPlugin,
  public IConnectionManager,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IConnectionManager IOptionsHolder);

public:
  ConnectionManager();
  ~ConnectionManager();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return CONNECTIONMANAGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);

  //IConnectionManager
  virtual QList<IConnectionPlugin *> pluginList() const { return FConnectionPlugins; }
  virtual IConnectionPlugin *pluginById(const QUuid &APluginId);
signals:
  virtual void connectionCreated(IConnection *AConnection);
  virtual void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS);
  virtual void connectionDestroyed(IConnection *AConnection);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  IConnectionPlugin *defaultPlugin() const;
protected slots:
  void onAccountAdded(IAccount *AAccount);
  void onAccountDestroyed(IAccount *AAccount);
  void onOptionsAccepted();
  void onOptionsRejected();
  void onOptionsDialogClosed();
private:
  IAccountManager *FAccountManager;
  ISettingsPlugin *FSettingsPlugin;
private:
  QList<IConnectionPlugin *> FConnectionPlugins;
  QList<ConnectionOptionsWidget *> FOptionsWidgets;
};

#endif 
