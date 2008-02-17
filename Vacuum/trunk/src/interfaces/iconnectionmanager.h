#ifndef ICONNECTIONMANAGER_H 
#define ICONNECTIONMANAGER_H 

#define CONNECTIONMANAGER_UUID "{B54F3B5E-3595-48c2-AB6F-249D4AD18327}"

#include <QObject>
#include <QUuid>

class IConnectionPlugin;

class IConnection
{
public:
  virtual QObject *instance() =0;
  virtual IConnectionPlugin *ownerPlugin() const =0;
  virtual bool isOpen() const =0;
  virtual void connectToHost() =0;
  virtual void disconnect() =0;
  virtual qint64 write(const QByteArray &AData) =0;
  virtual QByteArray read(qint64 ABytes) =0;
  virtual QVariant option(int ARole) const =0;
  virtual void setOption(int ARole, const QVariant &AValue) =0;
signals:
  virtual void connected() =0;
  virtual void readyRead(qint64 ABytes) =0;
  virtual void disconnected() =0;
  virtual void error(const QString &AMessage) =0;
};

class IConnectionPlugin
{
public:
  virtual QObject *instance() =0;
  virtual QUuid pluginUuid() const =0;
  virtual QString displayName() const =0;
  virtual IConnection *newConnection(const QString &ASettingsNS, QObject *AParent) =0;
  virtual void destroyConnection(IConnection *AConnection) =0;
  virtual void loadSettings(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void saveSettings(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void deleteSettingsNS(const QString &ASettingsNS) =0;
  virtual QWidget *optionsWidget(const QString &ASettingsNS) =0;
  virtual void saveOptions(const QString &ASettingsNS) =0;
signals:
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
signals:
  virtual void connectionCreated(IConnection *AConnection) =0;
  virtual void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS) =0;
  virtual void connectionDestroyed(IConnection *AConnection) =0;
};

Q_DECLARE_INTERFACE(IConnection,"Vacuum.Plugin.IConnection/1.0")
Q_DECLARE_INTERFACE(IConnectionPlugin,"Vacuum.Plugin.IConnectionPlugin/1.0")
Q_DECLARE_INTERFACE(IConnectionManager,"Vacuum.Plugin.IConnectionManager/1.0")

#endif 
