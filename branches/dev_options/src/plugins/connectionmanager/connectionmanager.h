#ifndef CONNECTIONMANAGER_H 
#define CONNECTIONMANAGER_H 

#include <QComboBox>
#include <definations/accountvaluenames.h>
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
#include <interfaces/isettings.h>
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
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IConnectionManager
  virtual QList<IConnectionPlugin *> pluginList() const;
  virtual IConnectionPlugin *pluginById(const QUuid &APluginId) const;
  virtual QList<QUuid> proxyList() const;
  virtual IConnectionProxy proxyById(const QUuid &AProxyId) const;
  virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  virtual void removeProxy(const QUuid &AProxyId);
  virtual QUuid defaultProxy() const;
  virtual void setDefaultProxy(const QUuid &AProxyId);
  virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL);
  virtual QWidget *proxySettingsWidget(const QString &ASettingsNS, QWidget *AParent);
  virtual void saveProxySettings(QWidget *AWidget, const QString &ASettingsNS = QString::null);
  virtual QUuid proxySettings(const QString &ASettingsNS) const;
  virtual void setProxySettings(const QString &ASettingsNS, const QUuid &AProxyId);
  virtual void deleteProxySettings(const QString &ASettingsNS);
signals:
  void connectionCreated(IConnection *AConnection);
  void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS);
  void connectionDestroyed(IConnection *AConnection);
  void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  void proxyRemoved(const QUuid &AProxyId);
  void defaultProxyChanged(const QUuid &AProxyId);
  void proxySettingsChanged(const QString &ASettingsNS, const QUuid &AProxyId);
  //IOptionsHolder
  void optionsAccepted();
  void optionsRejected();
public:
  IConnectionPlugin *defaultPlugin() const;
  IConnection *insertConnection(IAccount *AAccount) const;
protected slots:
  void onAccountShown(IAccount *AAccount);
  void onAccountDestroyed(const QUuid &AAccount);
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IAccountManager *FAccountManager;
  ISettingsPlugin *FSettingsPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  int FEncryptedLabelId;
  QList<IConnectionPlugin *> FPlugins;
private:
  QUuid FDefaultProxy;
  QMap<QUuid, IConnectionProxy> FProxys;
};

#endif // CONNECTIONMANAGER_H
