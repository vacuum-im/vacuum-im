#ifndef CONNECTIONMANAGER_H 
#define CONNECTIONMANAGER_H 

#include <QComboBox>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/rosterlabelorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/widgetmanager.h>
#include "editproxydialog.h"
#include "proxysettingswidget.h"
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
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IConnectionManager
  virtual QList<QString> pluginList() const;
  virtual IConnectionPlugin *pluginById(const QString &APluginId) const;
  virtual QList<QUuid> proxyList() const;
  virtual IConnectionProxy proxyById(const QUuid &AProxyId) const;
  virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  virtual void removeProxy(const QUuid &AProxyId);
  virtual QUuid defaultProxy() const;
  virtual void setDefaultProxy(const QUuid &AProxyId);
  virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL);
  virtual IOptionsWidget *proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent);
  virtual void saveProxySettings(QWidget *AWidget, OptionsNode ANode = OptionsNode::null);
  virtual QUuid loadProxySettings(const OptionsNode &ANode) const;
signals:
  void connectionCreated(IConnection *AConnection);
  void connectionDestroyed(IConnection *AConnection);
  void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  void proxyRemoved(const QUuid &AProxyId);
  void defaultProxyChanged(const QUuid &AProxyId);
public:
  IConnection *updateAccountConnection(IAccount *AAccount) const;
protected slots:
  void onAccountShown(IAccount *AAccount);
  void onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode);
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onOptionsOpened();
private:
  IAccountManager *FAccountManager;
  IOptionsManager *FOptionsManager;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  int FEncryptedLabelId;
  QMap<QString, IConnectionPlugin *> FPlugins;
};

#endif // CONNECTIONMANAGER_H
