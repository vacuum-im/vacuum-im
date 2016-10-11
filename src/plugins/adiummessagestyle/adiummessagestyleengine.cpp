#include "adiummessagestyleengine.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>
#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <utils/filestorage.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>

AdiumMessageStyleEngine::AdiumMessageStyleEngine()
{
	FUrlProcessor = NULL;
	FMessageStyleManager = NULL;
	FNetworkAccessManager = NULL;
}

AdiumMessageStyleEngine::~AdiumMessageStyleEngine()
{

}

void AdiumMessageStyleEngine::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Adium Message Style");
	APluginInfo->description = tr("Allows to use a Adium style in message design");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool AdiumMessageStyleEngine::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IMessageStyleManager").value(0);
	if (plugin)
	{
		FMessageStyleManager = qobject_cast<IMessageStyleManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IUrlProcessor").value(0);
	if (plugin)
	{
		FUrlProcessor = qobject_cast<IUrlProcessor *>(plugin->instance());
	}

	return true;
}

bool AdiumMessageStyleEngine::initObjects()
{
	FNetworkAccessManager = FUrlProcessor!=NULL ? FUrlProcessor->networkAccessManager() : new QNetworkAccessManager(this);
	updateAvailStyles();

	if (FMessageStyleManager)
	{
		FMessageStyleManager->registerStyleEngine(this);
	}

	return true;
}

QString AdiumMessageStyleEngine::engineId() const
{
	static const QString id = "AdiumMessageStyle";
	return id;
}

QString AdiumMessageStyleEngine::engineName() const
{
	return tr("Adium");
}

QList<QString> AdiumMessageStyleEngine::styles() const
{
	return FStylePaths.keys();
}

QList<int> AdiumMessageStyleEngine::supportedMessageTypes() const
{
	static const QList<int> messageTypes = QList<int>() << Message::Chat << Message::GroupChat;
	return messageTypes;
}

IMessageStyle *AdiumMessageStyleEngine::styleForOptions(const IMessageStyleOptions &AOptions)
{
	if (!FStyles.contains(AOptions.styleId))
	{
		QString stylePath = FStylePaths.value(AOptions.styleId);
		if (!stylePath.isEmpty())
		{
			AdiumMessageStyle *style = new AdiumMessageStyle(stylePath, FNetworkAccessManager, this);
			if (style->isValid())
			{
				LOG_INFO(QString("Adium style created, id=%1").arg(style->styleId()));
				FStyles.insert(AOptions.styleId,style);
				connect(style,SIGNAL(widgetAdded(QWidget *)),SLOT(onStyleWidgetAdded(QWidget *)));
				connect(style,SIGNAL(widgetRemoved(QWidget *)),SLOT(onStyleWidgetRemoved(QWidget *)));
				emit styleCreated(style);
			}
			else
			{
				delete style;
				REPORT_ERROR(QString("Failed to create adium style id=%1: Invalid style").arg(AOptions.styleId));
			}
		}
		else
		{
			REPORT_ERROR(QString("Failed to create adium style id=%1: Style not found").arg(AOptions.styleId));
		}
	}
	return FStyles.value(AOptions.styleId);
}

IMessageStyleOptions AdiumMessageStyleEngine::styleOptions(const OptionsNode &AEngineNode, const QString &AStyleId) const
{
	IMessageStyleOptions options;
	if (AStyleId.isEmpty() || FStylePaths.contains(AStyleId))
	{
		QString styleId = AStyleId.isEmpty() ? AEngineNode.value("style-id").toString() : AStyleId;

		// Select default style
		if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId))
		{
			int mtype = AEngineNode.parentNSpaces().value(1).toInt();
			switch (mtype)
			{
			case Message::GroupChat:
				styleId = "yMous";
				AEngineNode.node("style",styleId).setValue(QString("Mercurial XtraColor Both"),"variant");
				break;
			default:
				styleId = "Renkoo";
				AEngineNode.node("style",styleId).setValue(QString("Blue on Green"),"variant");
			}
			styleId = !FStylePaths.contains(styleId) ? FStylePaths.keys().first() : styleId;
		}

		if (FStylePaths.contains(styleId))
		{
			options.engineId = engineId();
			options.styleId = styleId;

			OptionsNode styleNode = AEngineNode.node("style",styleId);
			options.extended.insert(MSO_HEADER_TYPE,AdiumMessageStyle::HeaderNone);
			options.extended.insert(MSO_VARIANT,styleNode.value("variant"));
			options.extended.insert(MSO_FONT_FAMILY,styleNode.value("font-family"));
			options.extended.insert(MSO_FONT_SIZE,styleNode.value("font-size"));
			options.extended.insert(MSO_SELF_COLOR,styleNode.value("self-color"));
			options.extended.insert(MSO_CONTACT_COLOR,styleNode.value("contact-color"));
			options.extended.insert(MSO_BG_COLOR,styleNode.value("bg-color"));
			options.extended.insert(MSO_BG_IMAGE_FILE,styleNode.value("bg-image-file"));
			options.extended.insert(MSO_BG_IMAGE_LAYOUT,styleNode.value("bg-image-layout"));

			QList<QString> variants = styleVariants(styleId);
			QMap<QString,QVariant> info = styleInfo(styleId);

			if (!variants.contains(options.extended.value(MSO_VARIANT).toString()))
				options.extended.insert(MSO_VARIANT,info.value(MSIV_DEFAULT_VARIANT, variants.value(0)));

			if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
			{
				options.extended.remove(MSO_BG_IMAGE_FILE);
				options.extended.remove(MSO_BG_IMAGE_LAYOUT);
				options.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
			}
			else if (options.extended.value(MSO_BG_COLOR).toString().isEmpty())
			{
				options.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
			}

			if (options.extended.value(MSO_FONT_FAMILY).toString().isEmpty())
				options.extended.insert(MSO_FONT_FAMILY,info.value(MSIV_DEFAULT_FONT_FAMILY));
			if (options.extended.value(MSO_FONT_SIZE).toInt() == 0)
				options.extended.insert(MSO_FONT_SIZE,info.value(MSIV_DEFAULT_FONT_SIZE));

			if (options.extended.value(MSO_SELF_COLOR).toString().isEmpty())
				options.extended.insert(MSO_SELF_COLOR,info.value(MSIV_DEFAULT_SELF_COLOR,QColor(Qt::red).name()));
			if (options.extended.value(MSO_CONTACT_COLOR).toString().isEmpty())
				options.extended.insert(MSO_CONTACT_COLOR,info.value(MSIV_DEFAULT_CONTACT_COLOR,QColor(Qt::blue).name()));
		}
		else
		{
			REPORT_ERROR("Failed to find any suitable adium message style");
		}
	}
	else
	{
		REPORT_ERROR(QString("Failed to get adium style options for style=%1: Style not found").arg(AStyleId));
	}

	return options;
}

IOptionsDialogWidget *AdiumMessageStyleEngine::styleSettingsWidget(const OptionsNode &AStyleNode, QWidget *AParent)
{
	updateAvailStyles();
	return new AdiumOptionsWidget(this,AStyleNode,AParent);
}

IMessageStyleOptions AdiumMessageStyleEngine::styleSettinsOptions(IOptionsDialogWidget *AWidget) const
{
	AdiumOptionsWidget *widget = qobject_cast<AdiumOptionsWidget *>(AWidget->instance());
	return widget!=NULL ? widget->styleOptions() : IMessageStyleOptions();
}

QList<QString> AdiumMessageStyleEngine::styleVariants(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->variants();
	return AdiumMessageStyle::styleVariants(FStylePaths.value(AStyleId));
}

QMap<QString,QVariant> AdiumMessageStyleEngine::styleInfo(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->infoValues();
	return AdiumMessageStyle::styleInfo(FStylePaths.value(AStyleId));
}

void AdiumMessageStyleEngine::updateAvailStyles()
{
	foreach(const QString &substorage, FileStorage::availSubStorages(RSR_STORAGE_ADIUMMESSAGESTYLES, false))
	{
		QDir dir(FileStorage::subStorageDirs(RSR_STORAGE_ADIUMMESSAGESTYLES,substorage).value(0));
		if (dir.exists())
		{
			if (!FStylePaths.values().contains(dir.absolutePath()))
			{
				bool valid = QFile::exists(dir.absoluteFilePath("Contents/Info.plist"));
				valid = valid && (QFile::exists(dir.absoluteFilePath("Contents/Resources/Incoming/Content.html")) || QFile::exists(dir.absoluteFilePath("Contents/Resources/Content.html")));
				if (valid)
				{
					QMap<QString, QVariant> info = AdiumMessageStyle::styleInfo(dir.absolutePath());
					QString styleId = info.value(MSIV_NAME).toString();
					if (!styleId.isEmpty())
					{
						LOG_DEBUG(QString("Adium style added, id=%1").arg(styleId));
						FStylePaths.insert(styleId,dir.absolutePath());
					}
					else
					{
						LOG_WARNING(QString("Failed to add adium style from directory=%1: Style name is empty").arg(dir.absolutePath()));
					}
				}
				else
				{
					LOG_WARNING(QString("Failed to add adium style from directory=%1: Invalid style").arg(dir.absolutePath()));
				}
			}
		}
		else
		{
			LOG_WARNING(QString("Failed to find root substorage=%1 directory").arg(substorage));
		}
	}
}

void AdiumMessageStyleEngine::onStyleWidgetAdded(QWidget *AWidget)
{
	AdiumMessageStyle *style = qobject_cast<AdiumMessageStyle *>(sender());
	if (style)
		emit styleWidgetAdded(style,AWidget);
}

void AdiumMessageStyleEngine::onStyleWidgetRemoved(QWidget *AWidget)
{
	AdiumMessageStyle *style = qobject_cast<AdiumMessageStyle *>(sender());
	if (style)
	{
		if (style->styleWidgets().isEmpty())
			QTimer::singleShot(0,this,SLOT(onClearEmptyStyles()));
		emit styleWidgetRemoved(style,AWidget);
	}
}

void AdiumMessageStyleEngine::onClearEmptyStyles()
{
	QMap<QString, AdiumMessageStyle *>::iterator it = FStyles.begin();
	while (it != FStyles.end())
	{
		AdiumMessageStyle *style = it.value();
		if (style->styleWidgets().isEmpty())
		{
			LOG_INFO(QString("Adium style destroyed, id=%1").arg(style->styleId()));
			it = FStyles.erase(it);
			emit styleDestroyed(style);
			delete style;
		}
		else
		{
			++it;
		}
	}
}
