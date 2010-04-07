#ifndef ICONNECTIONMANAGER_H 
#define ICONNECTIONMANAGER_H 

#define CONNECTIONMANAGER_UUID "{B54F3B5E-3595-48c2-AB6F-249D4AD18327}"

#include <QUuid>
#include <QDialog>
#include <QNetworkProxy>

class IConnectionPlugin;

struct IConnectionProxy 
{
  QString name;
  QNetworkProxy proxy;
};

class IConnection
{
public:
  virtual QObject *instance() =0;
  virtual IConnectionPlugin *ownerPlugin() const =0;
  virtual bool isOpen() const =0;
  virtual bool isEncrypted() const =0;
  virtual bool connectToHost() =0;
  virtual void disconnectFromHost() =0;
  virtual qint64 write(const QByteArray &AData) =0;
  virtual QByteArray read(qint64 ABytes) =0;
  virtual QVariant option(int ARole) const =0;
  virtual void setOption(int ARole, const QVariant &AValue) =0;
protected:
  virtual void aboutToConnect() =0;
  virtual void connected() =0;
  virtual void encrypted() =0;
  virtual void readyRead(qint64 ABytes) =0;
  virtual void error(const QString &AMessage) =0;
  virtual void aboutToDisconnect() =0;
  virtual void disconnected() =0;
};

class IConnectionPlugin
{
public:
  virtual QObject *instance() =0;
  virtual QUuid pluginUuid() const =0;
  virtual QString displayName() const =0;
  virtual IConnection *newConnection(const QString &ASettingsNS, QObject *AParent) =0;
  virtual void destroyConnection(IConnection *AConnection) =0;
  virtual QWidget *settingsWidget(const QString &ASettingsNS) =0;
  virtual void saveSettings(QWidget *AWidget, const QString &ASettingsNS = QString::null) =0;
  virtual void loadSettings(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void deleteSettings(const QString &ASettingsNS) =0;
protected:
  virtual void connectionCreated(IConnection *AConnection) =0;
  virtual void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void connectionDestroyed(IConnection *AConnection) =0;
};

class IConnectionManager
{
public:
  virtual QObject *instance() =0;
  virtual QList<IConnectionPlugin *> pluginList() const =0;
  virtual IConnectionPlugin *pluginById(const QUuid &APluginId) const =0;
  virtual QList<QUuid> proxyList() const =0;
  virtual IConnectionProxy proxyById(const QUuid &AProxyId) const =0;
  virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
  virtual void removeProxy(const QUuid &AProxyId) =0;
  virtual QUuid defaultProxy() const =0;
  virtual void setDefaultProxy(const QUuid &AProxyId) =0;
  virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL) =0;
  virtual QWidget *proxySettingsWidget(const QString &ASettingsNS, QWidget *AParent) =0;
  virtual void saveProxySettings(QWidget *AWidget, const QString &ASettingsNS = QString::null) =0;
  virtual QUuid proxySettings(const QString &ASettingsNS) const =0;
  virtual void setProxySettings(const QString &ASettingsNS, const QUuid &AProxyId) =0;
  virtual void deleteProxySettings(const QString &ASettingsNS) =0;
protected:
  virtual void connectionCreated(IConnection *AConnection) =0;
  virtual void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void connectionDestroyed(IConnection *AConnection) =0;
  virtual void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
  virtual void proxyRemoved(const QUuid &AProxyId) =0;
  virtual void defaultProxyChanged(const QUuid &AProxyId) =0;
  virtual void proxySettingsChanged(const QString &ASettingsNS, const QUuid &AProxyId) =0;
};

Q_DECLARE_INTERFACE(IConnection,"Vacuum.Plugin.IConnection/1.0")
Q_DECLARE_INTERFACE(IConnectionPlugin,"Vacuum.Plugin.IConnectionPlugin/1.0")
Q_DECLARE_INTERFACE(IConnectionManager,"Vacuum.Plugin.IConnectionManager/1.0")

#endif 
