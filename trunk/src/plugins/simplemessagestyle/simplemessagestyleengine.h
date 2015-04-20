#ifndef SIMPLEMESSAGESTYLEPLUGIN_H
#define SIMPLEMESSAGESTYLEPLUGIN_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/iurlprocessor.h>
#include "simplemessagestyle.h"
#include "simpleoptionswidget.h"

#define SIMPLEMESSAGESTYLE_UUID   "{cfad7d10-58d0-4638-9940-dda64c1dd509}"

class SimpleMessageStyleEngine :
	public QObject,
	public IPlugin,
	public IMessageStyleEngine
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageStyleEngine);
public:
	SimpleMessageStyleEngine();
	~SimpleMessageStyleEngine();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SIMPLEMESSAGESTYLE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMessageStyleEngine
	virtual QString engineId() const;
	virtual QString engineName() const;
	virtual QList<QString> styles() const;
	virtual QList<int> supportedMessageTypes() const;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions);
	virtual IMessageStyleOptions styleOptions(const OptionsNode &AEngineNode, const QString &AStyleId=QString::null) const;
	virtual IOptionsDialogWidget *styleSettingsWidget(const OptionsNode &AStyleNode, QWidget *AParent);
	virtual IMessageStyleOptions styleSettinsOptions(IOptionsDialogWidget *AWidget) const;
	//SimpleMessageStyleEngine
	QList<QString> styleVariants(const QString &AStyleId) const;
	QMap<QString,QVariant> styleInfo(const QString &AStyleId) const;
signals:
	void styleCreated(IMessageStyle *AStyle) const;
	void styleDestroyed(IMessageStyle *AStyle) const;
	void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const;
	void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const;
protected:
	void updateAvailStyles();
protected slots:
	void onStyleWidgetAdded(QWidget *AWidget);
	void onStyleWidgetRemoved(QWidget *AWidget);
	void onClearEmptyStyles();
private:
	IUrlProcessor *FUrlProcessor;
	IMessageStyleManager *FMessageStyleManager;
private:
	QMap<QString, QString> FStylePaths;
	QMap<QString, SimpleMessageStyle *> FStyles;
	QNetworkAccessManager *FNetworkAccessManager;
};

#endif // SIMPLEMESSAGESTYLEPLUGIN_H
