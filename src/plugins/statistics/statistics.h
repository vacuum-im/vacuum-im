#ifndef STATISTICS_H
#define STATISTICS_H

#include <QMap>
#include <QTimer>
#include <QNetworkReply>
#include <QDesktopWidget>
#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istatistics.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iconnectionmanager.h>

class Statistics : 
	public QObject,
	public IPlugin,
	public IStatistics,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStatistics IOptionsDialogHolder);
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
protected slots:
	void onNetworkManagerFinished(QNetworkReply *AReply);
	void onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onPendingTimerTimeout();
	void onSessionTimerTimeout();
	void onDefaultConnectionProxyChanged(const QUuid &AProxyId);
protected slots:
	void onLoggerViewReported(const QString &AClass);
	void onLoggerErrorReported(const QString &AClass, const QString &AMessage, bool AFatal);
	void onLoggerEventReported(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue);
	void onLoggerTimingReported(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime);
private:
	IPluginManager *FPluginManager;
	IOptionsManager *FOptionsManager;
	IConnectionManager *FConnectionManager;
private:
	QUuid FProfileId;
	QDesktopWidget *FDesktopWidget;
	QNetworkAccessManager *FNetworkManager;
private:
	bool FSendHits;
	QString FUserAgent;
	QTimer FPendingTimer;
	QTimer FSessionTimer;
	QList<IStatisticsHit> FPendingHits;
	QMap<QNetworkReply *, IStatisticsHit> FReplyHits;
private:
	QMultiMap<QString,QString> FReportedErrors;
};

#endif // STATISTICS_H
