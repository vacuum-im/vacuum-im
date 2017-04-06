#ifndef STATISTICS_H
#define STATISTICS_H

#include <QMap>
#include <QTimer>
#include <QNetworkReply>
#include <QDesktopWidget>
#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibookmarks.h>
#include <interfaces/istatistics.h>
#include <interfaces/iclientinfo.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/ixmppstreammanager.h>

class Statistics : 
	public QObject,
	public IPlugin,
	public IStatistics,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStatistics IOptionsDialogHolder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.Statistics");
public:
	Statistics();
	~Statistics();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STATISTICS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	// IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	// IStatistics
	virtual QUuid profileId() const;
	virtual bool isValidHit(const IStatisticsHit &AHit) const;
	virtual bool sendStatisticsHit(const IStatisticsHit &AHit);
protected:
	QString userAgent() const;
	QString windowsVersion() const;
protected:
	QUrl buildHitUrl(const IStatisticsHit &AHit) const;
	QString getStatisticsFilePath(const QString &AFileName) const;
protected:
	IStatisticsHit makeViewHit() const;
	IStatisticsHit makeEventHit(const QString &AParams, int AValue=1) const;
	IStatisticsHit makeSessionEvent(const QString &AParams, int ASession) const;
	void sendServerInfoHit(const QString &AName, const QString &AVersion);
protected slots:
	void onPendingHitsTimerTimeout();
	void onNetworkManagerFinished(QNetworkReply *AReply);
	void onNetworkManagerSSLErrors(QNetworkReply *AReply, const QList<QSslError> &AErrors);
	void onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
	void onDefaultConnectionProxyChanged(const QUuid &AProxyId);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onSessionTimerTimeout();
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onSoftwareInfoChanged(const Jid &AContactJid);
protected slots:
	void onLoggerViewReported(const QString &AClass);
	void onLoggerErrorReported(const QString &AClass, const QString &AMessage, bool AFatal);
	void onLoggerEventReported(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue);
	void onLoggerTimingReported(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime);
private:
	IPluginManager *FPluginManager;
	IConnectionManager *FConnectionManager;
private:
	IBookmarks *FBookmarks;
	IClientInfo *FClientInfo;
	IServiceDiscovery *FDiscovery;
	IStatusChanger *FStatusChanger;
	IRosterManager *FRosterManager;
	IMessageWidgets *FMessageWidgets;
	IOptionsManager *FOptionsManager;
	IAccountManager *FAccountManager;
	IXmppStreamManager *FXmppStreamManager;
	IMultiUserChatManager *FMultiChatManager;
private:
	QMap<Jid,Jid> FSoftwareRequests;
private:
	QUuid FProfileId;
	QDesktopWidget *FDesktopWidget;
	QNetworkAccessManager *FNetworkManager;
private:
	bool FSendHits;
	QString FUserAgent;
	QString FClientVersion;
	QTimer FPendingTimer;
	QTimer FSessionTimer;
	QList<IStatisticsHit> FPendingHits;
	QMap<QNetworkReply *, IStatisticsHit> FReplyHits;
private:
	QMultiMap<QString,QString> FReportedErrors;
};

#endif // STATISTICS_H
