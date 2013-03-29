#ifndef STATISTICS_H
#define STATISTICS_H

#include <QWebView>
#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>

#define STATISTICS_UUID "{C9344821-9406-4089-A9D0-D6FD4919CF8F}"

class Statistics : 
	public QObject,
	public IPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin);
public:
	Statistics();
	~Statistics();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STATISTICS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
protected:
	QString getDataFilePath(const QString &AFileName) const;
protected slots:
	void onWebViewLoadFinished(bool AOk);
private:
	IPluginManager *FPluginManager;
private:
	QWebView *FStatisticsView;
};

#endif // STATISTICS_H
