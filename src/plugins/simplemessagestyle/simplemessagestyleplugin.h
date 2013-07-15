#ifndef SIMPLEMESSAGESTYLEPLUGIN_H
#define SIMPLEMESSAGESTYLEPLUGIN_H

#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/iurlprocessor.h>
#include <utils/message.h>
#include <utils/options.h>
#include "simplemessagestyle.h"
#include "simpleoptionswidget.h"

#define SIMPLEMESSAGESTYLE_UUID   "{cfad7d10-58d0-4638-9940-dda64c1dd509}"

class SimpleMessageStylePlugin :
			public QObject,
			public IPlugin,
			public IMessageStylePlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageStylePlugin);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IMessageStylePlugin");
#endif
public:
	SimpleMessageStylePlugin();
	~SimpleMessageStylePlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SIMPLEMESSAGESTYLE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMessageStylePlugin
	virtual QString pluginId() const;
	virtual QString pluginName() const;
	virtual QList<QString> styles() const;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions);
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const;
	virtual IOptionsWidget *styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent);
	virtual void saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions);
	//SimpleMessageStylePlugin
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
private:
	QMap<QString, QString> FStylePaths;
	QMap<QString, SimpleMessageStyle *> FStyles;
	QNetworkAccessManager *FNetworkAccessManager;
};

#endif // SIMPLEMESSAGESTYLEPLUGIN_H
