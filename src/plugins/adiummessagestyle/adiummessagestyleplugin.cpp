#include "adiummessagestyleplugin.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>

AdiumMessageStylePlugin::AdiumMessageStylePlugin()
{
	FUrlProcessor = NULL;
	FNetworkAccessManager = NULL;
}

AdiumMessageStylePlugin::~AdiumMessageStylePlugin()
{

}

void AdiumMessageStylePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Adium Message Style");
	APluginInfo->description = tr("Allows to use a Adium style in message design");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool AdiumMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IUrlProcessor").value(0);
	if (plugin)
	{
		FUrlProcessor = qobject_cast<IUrlProcessor *>(plugin->instance());
	}

	return true;
}

bool AdiumMessageStylePlugin::initObjects()
{
	FNetworkAccessManager = FUrlProcessor!=NULL ? FUrlProcessor->networkAccessManager() : new QNetworkAccessManager(this);
	updateAvailStyles();
	return true;
}

QString AdiumMessageStylePlugin::pluginId() const
{
	static const QString id = "AdiumMessageStyle";
	return id;
}

QString AdiumMessageStylePlugin::pluginName() const
{
	return tr("Adium Style");
}

QList<QString> AdiumMessageStylePlugin::styles() const
{
	return FStylePaths.keys();
}

IMessageStyle *AdiumMessageStylePlugin::styleForOptions(const IMessageStyleOptions &AOptions)
{
	QString styleId = AOptions.extended.value(MSO_STYLE_ID).toString();
	if (!FStyles.contains(styleId))
	{
		QString stylePath = FStylePaths.value(styleId);
		if (!stylePath.isEmpty())
		{
			AdiumMessageStyle *style = new AdiumMessageStyle(stylePath, FNetworkAccessManager, this);
			if (style->isValid())
			{
				FStyles.insert(styleId,style);
				connect(style,SIGNAL(widgetAdded(QWidget *)),SLOT(onStyleWidgetAdded(QWidget *)));
				connect(style,SIGNAL(widgetRemoved(QWidget *)),SLOT(onStyleWidgetRemoved(QWidget *)));
				emit styleCreated(style);
			}
			else
			{
				delete style;
			}
		}
	}
	return FStyles.value(styleId,NULL);
}

IMessageStyleOptions AdiumMessageStylePlugin::styleOptions(const OptionsNode &ANode, int AMessageType) const
{
	IMessageStyleOptions soptions;
	QVariant &styleId = soptions.extended[MSO_STYLE_ID];

	soptions.pluginId = pluginId();
	styleId = ANode.value("style-id").toString();
	soptions.extended.insert(MSO_HEADER_TYPE,AdiumMessageStyle::HeaderNone);
	soptions.extended.insert(MSO_VARIANT,ANode.value("variant"));
	soptions.extended.insert(MSO_FONT_FAMILY,ANode.value("font-family"));
	soptions.extended.insert(MSO_FONT_SIZE,ANode.value("font-size"));
	soptions.extended.insert(MSO_BG_COLOR,ANode.value("bg-color"));
	soptions.extended.insert(MSO_BG_IMAGE_FILE,ANode.value("bg-image-file"));
	soptions.extended.insert(MSO_BG_IMAGE_LAYOUT,ANode.value("bg-image-layout"));

	if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId.toString()))
	{
		switch (AMessageType)
		{
		case Message::GroupChat:
			styleId = QLatin1String("yMous");
			soptions.extended.insert(MSO_VARIANT,"Mercurial XtraColor Both");
			break;
		default:
			styleId = QLatin1String("Renkoo");
			soptions.extended.insert(MSO_VARIANT,"Blue on Green");
		}

		if (!FStylePaths.contains(styleId.toString()))
			styleId = FStylePaths.keys().first();
	}

	if (FStylePaths.contains(styleId.toString()))
	{
		QList<QString> variants = styleVariants(styleId.toString());
		QMap<QString,QVariant> info = styleInfo(styleId.toString());

		if (!variants.contains(soptions.extended.value(MSO_VARIANT).toString()))
			soptions.extended.insert(MSO_VARIANT,info.value(MSIV_DEFAULT_VARIANT, !variants.isEmpty() ? variants.first() : QString::null));

		if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
		{
			soptions.extended.remove(MSO_BG_IMAGE_FILE);
			soptions.extended.remove(MSO_BG_IMAGE_LAYOUT);
			soptions.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
		}
		else if (soptions.extended.value(MSO_BG_COLOR).toString().isEmpty())
		{
			soptions.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
		}

		if (soptions.extended.value(MSO_FONT_FAMILY).toString().isEmpty())
			soptions.extended.insert(MSO_FONT_FAMILY,info.value(MSIV_DEFAULT_FONT_FAMILY));
		if (soptions.extended.value(MSO_FONT_SIZE).toInt()==0)
			soptions.extended.insert(MSO_FONT_SIZE,info.value(MSIV_DEFAULT_FONT_SIZE));
	}
	return soptions;
}

IOptionsWidget *AdiumMessageStylePlugin::styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent)
{
	updateAvailStyles();
	return new AdiumOptionsWidget(this,ANode,AMessageType,AParent);
}

void AdiumMessageStylePlugin::saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
	AdiumOptionsWidget *widget = qobject_cast<AdiumOptionsWidget *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
}

void AdiumMessageStylePlugin::saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions)
{
	AdiumOptionsWidget *widget = qobject_cast<AdiumOptionsWidget *>(AWidget->instance());
	if (widget)
		AOptions = widget->styleOptions();
}

QList<QString> AdiumMessageStylePlugin::styleVariants(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->variants();
	return AdiumMessageStyle::styleVariants(FStylePaths.value(AStyleId));
}

QMap<QString,QVariant> AdiumMessageStylePlugin::styleInfo(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->infoValues();
	return AdiumMessageStyle::styleInfo(FStylePaths.value(AStyleId));
}

void AdiumMessageStylePlugin::updateAvailStyles()
{
	foreach(QString substorage, FileStorage::availSubStorages(RSR_STORAGE_ADIUMMESSAGESTYLES, false))
	{
		QDir dir(FileStorage::subStorageDirs(RSR_STORAGE_ADIUMMESSAGESTYLES,substorage).value(0));
		if (dir.exists())
		{
			if (!FStylePaths.values().contains(dir.absolutePath()))
			{
				bool valid = QFile::exists(dir.absoluteFilePath("Contents/Info.plist"));
				valid = valid &&  QFile::exists(dir.absoluteFilePath("Contents/Resources/Incoming/Content.html"));
				if (valid)
				{
					QMap<QString, QVariant> info = AdiumMessageStyle::styleInfo(dir.absolutePath());
					if (!info.value(MSIV_NAME).toString().isEmpty())
						FStylePaths.insert(info.value(MSIV_NAME).toString(),dir.absolutePath());
				}
			}
		}
	}
}

void AdiumMessageStylePlugin::onStyleWidgetAdded(QWidget *AWidget)
{
	AdiumMessageStyle *style = qobject_cast<AdiumMessageStyle *>(sender());
	if (style)
		emit styleWidgetAdded(style,AWidget);
}

void AdiumMessageStylePlugin::onStyleWidgetRemoved(QWidget *AWidget)
{
	AdiumMessageStyle *style = qobject_cast<AdiumMessageStyle *>(sender());
	if (style)
	{
		if (style->styleWidgets().isEmpty())
			QTimer::singleShot(0,this,SLOT(onClearEmptyStyles()));
		emit styleWidgetRemoved(style,AWidget);
	}
}

void AdiumMessageStylePlugin::onClearEmptyStyles()
{
	QMap<QString, AdiumMessageStyle *>::iterator it = FStyles.begin();
	while (it!=FStyles.end())
	{
		AdiumMessageStyle *style = it.value();
		if (style->styleWidgets().isEmpty())
		{
			it = FStyles.erase(it);
			emit styleDestroyed(style);
			delete style;
		}
		else
			++it;
	}
}

Q_EXPORT_PLUGIN2(plg_adiummessagestyle, AdiumMessageStylePlugin)
