#ifndef SOCKSSTREAMS_H
#define SOCKSSTREAMS_H

#include <QTcpServer>
#include <definations/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isocksstreams.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/isettings.h>
#include "socksstream.h"
#include "socksoptions.h"

class SocksStreams : 
  public QObject,
  public IPlugin,
  public ISocksStreams
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ISocksStreams);
public:
  SocksStreams();
  ~SocksStreams();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SOCKSSTREAMS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IDataStreamMethod
  virtual QString methodNS() const;
  virtual QString methodName() const;
  virtual QString methodDescription() const;
  virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, 
    const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent=NULL);
  virtual QWidget *settingsWidget(IDataStreamSocket *ASocket, bool AReadOnly);
  virtual QWidget *settingsWidget(const QString &ASettingsNS, bool AReadOnly);
  virtual void loadSettings(IDataStreamSocket *ASocket, QWidget *AWidget) const;
  virtual void loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS) const;
  virtual void saveSettings(const QString &ASettingsNS, QWidget *AWidget);
  virtual void saveSettings(const QString &ASettingsNS, IDataStreamSocket *ASocket);
  virtual void deleteSettings(const QString &ASettingsNS);
  //ISocksStreams
  virtual quint16 serverPort() const;
  virtual void setServerPort(quint16 APort);
  virtual bool disableDirectConnections(const QString &ASettingsNS) const;
  virtual void setDisableDirectConnections(const QString &ASettingsNS, bool ADisable);
  virtual bool useAccountNetworkProxy(const QString &ASettingsNS) const;
  virtual void setUseAccountNetworkProxy(const QString &ASettingsNS, bool AUse);
  virtual bool useNativeServerProxy(const QString &ASettingsNS) const;
  virtual void setUseNativeServerProxy(const QString &ASettingsNS, bool AAppend);
  virtual QString forwardHost(const QString &ASettingsNS) const;
  virtual quint16 forwardPort(const QString &ASettingsNS) const;
  virtual void setForwardAddress(const QString &ASettingsNS, const QString &AHost, quint16 APort);
  virtual QNetworkProxy accountNetworkProxy(const Jid &AStreamJid) const;
  virtual QNetworkProxy networkProxy(const QString &ASettingsNS) const;
  virtual void setNetworkProxy(const QString &ASettingsNS, const QNetworkProxy &AProxy);
  virtual QString nativeServerProxy(const Jid &AStreamJid) const;
  virtual QList<QString> proxyList(const QString &ASettingsNS) const;
  virtual void setProxyList(const QString &ASettingsNS, const QList<QString> &AProxyList);
  virtual QString connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const;
  virtual bool appendLocalConnection(const QString &AKey);
  virtual void removeLocalConnection(const QString &AKey);
signals:
  virtual void localConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket);
  //IDataStreamMethod
  virtual void socketCreated(IDataStreamSocket *ASocket);
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
  void onXmppStreamOpened(IXmppStream *AStream);
  void onXmppStreamClosed(IXmppStream *AStream);
  void onDiscoItemsReceived(const IDiscoItems &AItems);
  void onNewServerConnection();
  void onServerConnectionReadyRead();
  void onServerConnectionDisconnected();
private:
  IXmppStreams *FXmppStreams;
  IDataStreamsManager *FDataManager;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;
private:
  quint16 FServerPort;
  bool FDisableDirectConnect;
  bool FUseNativeServerProxy;
  bool FUseAccountNetworkProxy;
  QString FForwardHost;
  quint16 FForwardPort;
  QNetworkProxy FNetworkProxy;
  QList<QString> FProxyList;
private:
  QTcpServer FServer;
  QList<QString> FLocalKeys;
  QMap<Jid, QString> FNativeProxy;
};

#endif // SOCKSSTREAMS_H
