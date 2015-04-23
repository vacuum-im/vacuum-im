#ifndef ADIUMMESSAGESTYLEPLUGIN_H
#define ADIUMMESSAGESTYLEPLUGIN_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/iurlprocessor.h>
#include "adiummessagestyle.h"
#include "adiumoptionswidget.h"

#define ADIUMMESSAGESTYLE_UUID    "{703bae73-1905-4840-a186-c70b359d4f21}"

class AdiumMessageStyleEngine :
	public QObject,
	public IPlugin,
	public IMessageStyleEngine
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageStyleEngine);
public:
	AdiumMessageStyleEngine();
	~AdiumMessageStyleEngine();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return ADIUMMESSAGESTYLE_UUID; }
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
	//AdiumMessageStyleEngine
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
	QMap<QString, AdiumMessageStyle *> FStyles;
	QNetworkAccessManager *FNetworkAccessManager;
};

#endif // ADIUMMESSAGESTYLEPLUGIN_H
