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
  virtual QList<IConnectionPlugin *> pluginList() const { return FConnectionPlugins; }
  virtual IConnectionPlugin *pluginById(const QUuid &APluginId) const;
signals:
  virtual void connectionCreated(IConnection *AConnection);
  virtual void connectionUpdated(IConnection *AConnection, const QString &ASettingsNS);
  virtual void connectionDestroyed(IConnection *AConnection);
signals:
  virtual void optionsAccepted();
  virtual void optionsRejected();
public:
  IConnectionPlugin *defaultPlugin() const;
  IConnection *insertConnection(IAccount *AAccount) const;
protected slots:
  void onAccountShown(IAccount *AAccount);
  void onAccountDestroyed(const QUuid &AAccount);
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
private:
  IAccountManager *FAccountManager;
  ISettingsPlugin *FSettingsPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  int FEncryptedLabelId;
  QList<IConnectionPlugin *> FConnectionPlugins;
};

#endif 
