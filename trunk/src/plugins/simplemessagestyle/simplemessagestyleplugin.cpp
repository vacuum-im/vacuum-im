#include "simplemessagestyleplugin.h"

#include <QDir>
#include <QCoreApplication>

#define SVN_STYLE                     "style%1[]"
#define SVN_STYLE_ID                  SVN_STYLE ":" "styleId"
#define SVN_STYLE_VARIANT             SVN_STYLE ":" "variant"
#define SVN_STYLE_BG_COLOR            SVN_STYLE ":" "bgColor"
#define SVN_STYLE_BG_IMAGE_FILE       SVN_STYLE ":" "bgImageFile"

SimpleMessageStylePlugin::SimpleMessageStylePlugin()
{
  FSettingsPlugin = NULL;
}

SimpleMessageStylePlugin::~SimpleMessageStylePlugin()
{

}

void SimpleMessageStylePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements basic message styles.");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Simple Message Style"); 
  APluginInfo->uid = SIMPLEMESSAGESTYLE_UUID;
  APluginInfo->version = "0.1";
}

bool SimpleMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
  return true;
}

bool SimpleMessageStylePlugin::initObjects()
{
  updateAvailStyles();
  return true;
}

QString SimpleMessageStylePlugin::stylePluginId() const
{
  static QString id = "SimpleMessageStyle";
  return id;
}

QList<QString> SimpleMessageStylePlugin::styles() const
{
  return FStylePaths.keys();
}

IMessageStyle * SimpleMessageStylePlugin::styleById(const QString &AStyleId)
{
  if (!FStyles.contains(AStyleId))
  {
    QString stylePath = FStylePaths.value(AStyleId);
    if (!stylePath.isEmpty())
    {
      IMessageStyle *style = new SimpleMessageStyle(stylePath,this);
      if (style->isValid())
      {
        FStyles.insert(AStyleId,style);
        emit styleCreated(style);
      }
      else
      {
        style->instance()->deleteLater();
      }
    }
  }
  return FStyles.value(AStyleId,NULL);
}

IMessageStyleOptions SimpleMessageStylePlugin::styleOptions(int AMessageType, const QString &AContext) const
{
  IMessageStyleOptions options;

  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    options.pluginId = stylePluginId();
    options.styleId = settings->valueNS(QString(SVN_STYLE_ID).arg(AMessageType),AContext).toString();
    options.options.insert(MSO_VARIANT,settings->valueNS(QString(SVN_STYLE_VARIANT).arg(AMessageType),AContext).toString());
    options.options.insert(MSO_BG_COLOR,settings->valueNS(QString(SVN_STYLE_BG_COLOR).arg(AMessageType),AContext).toString());
    options.options.insert(MSO_BG_IMAGE_FILE,settings->valueNS(QString(SVN_STYLE_BG_IMAGE_FILE).arg(AMessageType),AContext).toString());
  }

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(options.styleId))
  {
    switch (AMessageType)
    {
    case Message::Normal:
    case Message::Headline:
      options.styleId = "Simple style for normal messages";
      break;
    case Message::GroupChat:
      options.styleId = "Simple style for groupchat";
      break;
    default:
      options.styleId = "Simple style for chat";
    }

    if (!FStylePaths.contains(options.styleId))  
      options.styleId = FStylePaths.keys().first();

    QString stylePath = FStylePaths.value(options.styleId);
    QList<QString> variants = SimpleMessageStyle::styleVariants(stylePath);
    QMap<QString,QVariant> info = SimpleMessageStyle::styleInfo(stylePath);
    if (!variants.contains(options.options.value(MSO_VARIANT).toString()))
      options.options.insert(MSO_VARIANT,info.value(MSIV_DEFAULT_VARIANT));

    if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
      options.options.remove(MSO_BG_IMAGE_FILE);
    options.options.remove(MSO_BG_COLOR);
  }

  return options;
}

void SimpleMessageStylePlugin::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (FSettingsPlugin && FStylePaths.contains(AOptions.styleId))
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    settings->setValueNS(QString(SVN_STYLE_ID).arg(AMessageType),AContext,AOptions.styleId);
    settings->setValueNS(QString(SVN_STYLE_VARIANT).arg(AMessageType),AContext,AOptions.options.value(MSO_VARIANT));
    settings->setValueNS(QString(SVN_STYLE_BG_COLOR).arg(AMessageType),AContext,AOptions.options.value(MSO_BG_COLOR));
    settings->setValueNS(QString(SVN_STYLE_BG_IMAGE_FILE).arg(AMessageType),AContext,AOptions.options.value(MSO_BG_IMAGE_FILE));
  }
  emit styleOptionsChanged(AOptions,AMessageType,AContext);
}

void SimpleMessageStylePlugin::updateAvailStyles()
{
  FStylePaths.clear();
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR) && dir.cd(RSR_STORAGE_SIMPLEMESSAGESTYLES))
  {
    QStringList styleDirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    styleDirs.removeAt(styleDirs.indexOf(STORAGE_SHARED_DIR));
    foreach(QString styleDir, styleDirs)
    {
      if (dir.cd(styleDir))
      {
        bool valid = QFile::exists(dir.absoluteFilePath("Info.plist"));
        valid = valid &&  QFile::exists(dir.absoluteFilePath("Incoming/Content.html"));
        if (valid)
        {
          QMap<QString, QVariant> info = SimpleMessageStyle::styleInfo(dir.absolutePath());
          if (!info.value(MSIV_NAME).toString().isEmpty())
            FStylePaths.insert(info.value(MSIV_NAME).toString(),dir.absolutePath());
        }
        dir.cdUp();
      }
    }
  }
}

Q_EXPORT_PLUGIN2(SimpleMessageStylePlugin, SimpleMessageStylePlugin)
