#ifndef ISOCKSSTREAMS_H
#define ISOCKSSTREAMS_H

#include <QIODevice>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <interfaces/idatastreamsmanager.h>

#define SOCKSSTREAMS_UUID       "{6c7cb01e-64c5-4644-97ba-1d00b8c417c2}"

class ISocksStream :
  public IDataStreamSocket
{
public:
  virtual QIODevice *instance() =0;
  virtual bool disableDirectConnection() const =0;
  virtual void setDisableDirectConnection(bool ADisable) =0;
  virtual QString forwardHost() const =0;
  virtual quint16 forwardPort() const =0;
  virtual void setForwardAddress(const QString &AHost, quint16 APort) =0;
  virtual QNetworkProxy networkProxy() const =0;
  virtual void setNetworkProxy(const QNetworkProxy &AProxy) =0;
  virtual QList<QString> proxyList() const =0;
  virtual void setProxyList(const QList<QString> &AProxyList) =0;
protected:
  virtual void propertiesChanged() =0;
};

class ISocksStreams :
  public IDataStreamMethod
{
public:
  virtual QObject *instance() =0;
  virtual quint16 serverPort() const =0;
  virtual void setServerPort(quint16 APort) =0;
  virtual bool disableDirectConnections(const QString &ASettingsNS) const =0;
  virtual void setDisableDirectConnections(const QString &ASettingsNS, bool ADisable) =0;
  virtual bool useAccountNetworkProxy(const QString &ASettingsNS) const =0;
  virtual void setUseAccountNetworkProxy(const QString &ASettingsNS, bool AUse) =0;
  virtual bool useNativeServerProxy(const QString &ASettingsNS) const =0;
  virtual void setUseNativeServerProxy(const QString &ASettingsNS, bool AUse) =0;
  virtual QString forwardHost(const QString &ASettingsNS) const =0;
  virtual quint16 forwardPort(const QString &ASettingsNS) const =0;
  virtual void setForwardAddress(const QString &ASettingsNS, const QString &AHost, quint16 APort) =0;
  virtual QNetworkProxy accountNetworkProxy(const Jid &AStreamJid) const =0;
  virtual QNetworkProxy networkProxy(const QString &ASettingsNS) const =0;
  virtual void setNetworkProxy(const QString &ASettingsNS, const QNetworkProxy &AProxy) =0;
  virtual QString nativeServerProxy(const Jid &AStreamJid) const =0;
  virtual QList<QString> proxyList(const QString &ASettingsNS) const =0;
  virtual void setProxyList(const QString &ASettingsNS, const QList<QString> &AProxyList) =0;
  virtual QString connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const =0;
  virtual bool appendLocalConnection(const QString &AKey) =0;
  virtual void removeLocalConnection(const QString &AKey) =0;
protected:
  virtual void localConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket) =0;
};

Q_DECLARE_INTERFACE(ISocksStream,"Vacuum.Plugin.ISocksStream/1.0")
Q_DECLARE_INTERFACE(ISocksStreams,"Vacuum.Plugin.ISocksStreams/1.0")

#endif // ISOCKSSTREAMS_H
