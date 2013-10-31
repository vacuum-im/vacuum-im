#ifndef STATISTICS_H
#define STATISTICS_H

#include <QMap>
#include <QTimer>
#include <QNetworkReply>
#include <QDesktopWidget>
#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istatistics.h>
#include <interfaces/iconnectionmanager.h>

class Statistics : 
	public QObject,
	public IPlugin,
	public IStatistics
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStatistics);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.Statistics");
public:
	Statistics();
	~Statistics();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STATISTICS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
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
protected slots:
	void onNetworkManagerFinished(QNetworkReply *AReply);
	void onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onPendingTimerTimeout();
	void onDefaultConnectionProxyChanged(const QUuid &AProxyId);
private:
	IPluginManager *FPluginManager;
	IConnectionManager *FConnectionManager;
private:
	QUuid FProfileId;
	QDesktopWidget *FDesktopWidget;
	QNetworkAccessManager *FNetworkManager;
private:
	QString FUserAgent;
	QTimer FPendingTimer;
	QList<IStatisticsHit> FPendingHits;
	QMap<QNetworkReply *, IStatisticsHit> FReplyHits;
};

#endif // STATISTICS_H