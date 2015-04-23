#include "simplemessagestyleengine.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>
#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <utils/filestorage.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>

SimpleMessageStyleEngine::SimpleMessageStyleEngine(): FNetworkAccessManager(NULL)
{
	FUrlProcessor = NULL;
	FMessageStyleManager = NULL;
	FNetworkAccessManager = NULL;
}

SimpleMessageStyleEngine::~SimpleMessageStyleEngine()
{

}

void SimpleMessageStyleEngine::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Simple Message Style");
	APluginInfo->description = tr("Allows to use a simplified style in message design");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool SimpleMessageStyleEngine::initConnections(IPluginManager *APluginManager, int &AInitOrder)
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

bool SimpleMessageStyleEngine::initObjects()
{
	FNetworkAccessManager = FUrlProcessor!=NULL ? FUrlProcessor->networkAccessManager() : new QNetworkAccessManager(this);
	updateAvailStyles();

	if (FMessageStyleManager)
	{
		FMessageStyleManager->registerStyleEngine(this);
	}

	return true;
}

QString SimpleMessageStyleEngine::engineId() const
{
	static QString id = "SimpleMessageStyle";
	return id;
}

QString SimpleMessageStyleEngine::engineName() const
{
	return tr("Simple");
}

QList<QString> SimpleMessageStyleEngine::styles() const
{
	return FStylePaths.keys();
}

QList<int> SimpleMessageStyleEngine::supportedMessageTypes() const
{
	static const QList<int> messageTypes = QList<int>() << Message::Chat << Message::GroupChat << Message::Normal << Message::Headline << Message::Error;
	return messageTypes;
}

IMessageStyle *SimpleMessageStyleEngine::styleForOptions(const IMessageStyleOptions &AOptions)
{
	if (!FStyles.contains(AOptions.styleId))
	{
		QString stylePath = FStylePaths.value(AOptions.styleId);
		if (!stylePath.isEmpty())
		{
			SimpleMessageStyle *style = new SimpleMessageStyle(stylePath, FNetworkAccessManager, this);
			if (style->isValid())
			{
				LOG_INFO(QString("Simple style created, id=%1").arg(style->styleId()));
				FStyles.insert(AOptions.styleId,style);
				connect(style,SIGNAL(widgetAdded(QWidget *)),SLOT(onStyleWidgetAdded(QWidget *)));
				connect(style,SIGNAL(widgetRemoved(QWidget *)),SLOT(onStyleWidgetRemoved(QWidget *)));
				emit styleCreated(style);
			}
			else
			{
				delete style;
				REPORT_ERROR(QString("Failed to create simple style id=%1: Invalid style").arg(AOptions.styleId));
			}
		}
		else
		{
			REPORT_ERROR(QString("Failed to create simple style id=%1: Style not found").arg(AOptions.styleId));
		}
	}
	return FStyles.value(AOptions.styleId);
}

IMessageStyleOptions SimpleMessageStyleEngine::styleOptions(const OptionsNode &AEngineNode, const QString &AStyleId) const
{
	IMessageStyleOptions soptions;
	if (AStyleId.isEmpty() || FStylePaths.contains(AStyleId))
	{
		QString styleId = AStyleId.isEmpty() ? AEngineNode.value("style-id").toString() : AStyleId;

		// Select default style
		if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId))
		{
			int mtype = AEngineNode.parentNSpaces().value(1).toInt();
			switch (mtype)
			{
			case Message::Normal:
			case Message::Headline:
			case Message::Error:
				styleId = "Message Style";
				AEngineNode.node("style",styleId).setValue(QString("Default"),"variant");
				break;
			default:
				styleId = "Chat Style";
				AEngineNode.node("style",styleId).setValue(QString("Default"),"variant");
			}
			styleId = !FStylePaths.contains(styleId) ? FStylePaths.keys().first() : styleId;
		}

		if (FStylePaths.contains(styleId))
		{
			soptions.engineId = engineId();
			soptions.styleId = styleId;

			OptionsNode styleNode = AEngineNode.node("style",styleId);
			soptions.extended.insert(MSO_VARIANT,styleNode.value("variant"));
			soptions.extended.insert(MSO_FONT_FAMILY,styleNode.value("font-family"));
			soptions.extended.insert(MSO_FONT_SIZE,styleNode.value("font-size"));
			soptions.extended.insert(MSO_BG_COLOR,styleNode.value("bg-color"));
			soptions.extended.insert(MSO_BG_IMAGE_FILE,styleNode.value("bg-image-file"));

			QList<QString> variants = styleVariants(styleId);
			QMap<QString,QVariant> info = styleInfo(styleId);

			if (!variants.contains(soptions.extended.value(MSO_VARIANT).toString()))
				soptions.extended.insert(MSO_VARIANT,info.value(MSIV_DEFAULT_VARIANT, variants.value(0)));

			if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
			{
				soptions.extended.remove(MSO_BG_IMAGE_FILE);
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
		else
		{
			REPORT_ERROR("Failed to find any suitable simple message style");
		}
	}
	else
	{
		REPORT_ERROR(QString("Failed to get adium style options for style=%1: Style not found").arg(AStyleId));
	}

	return soptions;
}

IOptionsDialogWidget *SimpleMessageStyleEngine::styleSettingsWidget(const OptionsNode &AStyleNode, QWidget *AParent)
{
	updateAvailStyles();
	return new SimpleOptionsWidget(this,AStyleNode,AParent);
}

IMessageStyleOptions SimpleMessageStyleEngine::styleSettinsOptions( IOptionsDialogWidget *AWidget ) const
{
	SimpleOptionsWidget *widget = qobject_cast<SimpleOptionsWidget *>(AWidget->instance());
	return widget!=NULL ? widget->styleOptions() : IMessageStyleOptions();
}

QList<QString> SimpleMessageStyleEngine::styleVariants(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->variants();
	return SimpleMessageStyle::styleVariants(FStylePaths.value(AStyleId));
}

QMap<QString,QVariant> SimpleMessageStyleEngine::styleInfo(const QString &AStyleId) const
{
	if (FStyles.contains(AStyleId))
		return FStyles.value(AStyleId)->infoValues();
	return SimpleMessageStyle::styleInfo(FStylePaths.value(AStyleId));
}

void SimpleMessageStyleEngine::updateAvailStyles()
{
	foreach(const QString &substorage, FileStorage::availSubStorages(RSR_STORAGE_SIMPLEMESSAGESTYLES, false))
	{
		QDir dir(FileStorage::subStorageDirs(RSR_STORAGE_SIMPLEMESSAGESTYLES,substorage).value(0));
		if (dir.exists())
		{
			if (!FStylePaths.values().contains(dir.absolutePath()))
			{
				bool valid = QFile::exists(dir.absoluteFilePath("Info.plist"));
				valid = valid &&  QFile::exists(dir.absoluteFilePath("Incoming/Content.html"));
				if (valid)
				{
					QMap<QString, QVariant> info = SimpleMessageStyle::styleInfo(dir.absolutePath());
					QString styleId = info.value(MSIV_NAME).toString();
					if (!styleId.isEmpty())
					{
						LOG_DEBUG(QString("Simple style added, id=%1").arg(styleId));
						FStylePaths.insert(styleId,dir.absolutePath());
					}
					else
					{
						LOG_WARNING(QString("Failed to add simple style from directory=%1: Style name is empty").arg(dir.absolutePath()));
					}
				}
				else
				{
					LOG_WARNING(QString("Failed to add simple style from directory=%1: Invalid style").arg(dir.absolutePath()));
				}
			}
		}
		else
		{
			LOG_WARNING(QString("Failed to find root substorage=%1 directory").arg(substorage));
		}
	}
}

void SimpleMessageStyleEngine::onStyleWidgetAdded(QWidget *AWidget)
{
	SimpleMessageStyle *style = qobject_cast<SimpleMessageStyle *>(sender());
	if (style)
		emit styleWidgetAdded(style,AWidget);
}

void SimpleMessageStyleEngine::onStyleWidgetRemoved(QWidget *AWidget)
{
	SimpleMessageStyle *style = qobject_cast<SimpleMessageStyle *>(sender());
	if (style)
	{
		if (style->styleWidgets().isEmpty())
			QTimer::singleShot(0,this,SLOT(onClearEmptyStyles()));
		emit styleWidgetRemoved(style,AWidget);
	}
}

void SimpleMessageStyleEngine::onClearEmptyStyles()
{
	QMap<QString, SimpleMessageStyle *>::iterator it = FStyles.begin();
	while (it!=FStyles.end())
	{
		SimpleMessageStyle *style = it.value();
		if (style->styleWidgets().isEmpty())
		{
			LOG_INFO(QString("Simple style destroyed, id=%1").arg(style->styleId()));
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
