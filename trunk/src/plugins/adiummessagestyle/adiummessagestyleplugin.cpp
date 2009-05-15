#include "adiummessagestyleplugin.h"

#include <QDir>
#include <QCoreApplication>

#define SVN_STYLE                     "style%1[]"
#define SVN_STYLE_ID                  SVN_STYLE ":" "styleId"
#define SVN_STYLE_VARIANT             SVN_STYLE ":" "variant"
#define SVN_STYLE_BG_COLOR            SVN_STYLE ":" "bgColor"
#define SVN_STYLE_BG_IMAGE_FILE       SVN_STYLE ":" "bgImageFile"
#define SVN_STYLE_BG_IMAGE_LAYOUT     SVN_STYLE ":" "bgImageLayout"

AdiumMessageStylePlugin::AdiumMessageStylePlugin()
{
  FSettingsPlugin = NULL;
}

AdiumMessageStylePlugin::~AdiumMessageStylePlugin()
{

}

void AdiumMessageStylePlugin::pluginInfo( IPluginInfo *APluginInfo )
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements support for Adium message styles");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Adiun Message Style"); 
  APluginInfo->uid = ADIUMMESSAGESTYLE_UUID;
  APluginInfo->version = "0.1";
}

bool AdiumMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
  return true;
}

bool AdiumMessageStylePlugin::initObjects()
{
  updateAvailStyles();
  return true;
}

QString & AdiumMessageStylePlugin::stylePluginId() const
{
  static QString id = "AdiumMessageStyle";
  return id;
}

QList<QString> AdiumMessageStylePlugin::styles() const
{
  return FStylePaths.keys();
}

IMessageStyle *AdiumMessageStylePlugin::styleById(const QString &AStyleId)
{
  if (!FStyles.contains(AStyleId))
  {
    QString stylePath = FStylePaths.value(AStyleId);
    if (!stylePath.isEmpty())
    {
      IMessageStyle *style = new AdiumMessageStyle(stylePath,this);
      if (style->isValid())
      {
        FStyles.insert(AStyleId,style);
        emit styleCreated(style);
      }
      else
      {
        delete style;
      }
    }
  }
  return FStyles.value(AStyleId,NULL);
}

IMessageStyleOptions AdiumMessageStylePlugin::styleOptions(int AMessageType, const QString &AContext) const
{
  IMessageStyleOptions options;

  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    options.pluginId = stylePluginId();
    options.styleId = settings->valueNS(QString(SVN_STYLE_ID).arg(AMessageType),AContext).toString();
    options.options.insert(MSO_HEADER_TYPE,AdiumMessageStyle::HeaderNone);
    options.options.insert(MSO_VARIANT,settings->valueNS(QString(SVN_STYLE_VARIANT).arg(AMessageType),AContext).toString());
    options.options.insert(MSO_BG_COLOR,settings->valueNS(QString(SVN_STYLE_BG_COLOR).arg(AMessageType),AContext).toString());
    options.options.insert(MSO_BG_IMAGE_FILE,settings->valueNS(QString(SVN_STYLE_BG_IMAGE_FILE).arg(AMessageType),AContext).toString());
    options.options.insert(MSO_BG_IMAGE_LAYOUT,settings->valueNS(QString(SVN_STYLE_BG_IMAGE_LAYOUT).arg(AMessageType),AContext).toString());
  }

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(options.styleId))
  {
    options.styleId = "yMous";
    switch (AMessageType)
    {
    case Message::GroupChat:
      options.options.insert(MSO_VARIANT,"Saturnine XtraColor Both");
      break;
    default:
      options.options.insert(MSO_VARIANT,"Saturnine Both");
    }

    if (!FStylePaths.contains(options.styleId))  
      options.styleId = FStylePaths.keys().first();

    QString stylePath = FStylePaths.value(options.styleId);
    QList<QString> variants = AdiumMessageStyle::styleVariants(stylePath);
    QMap<QString,QVariant> info = AdiumMessageStyle::styleInfo(stylePath);
    if (!variants.contains(options.options.value(MSO_VARIANT).toString()))
      options.options.insert(MSO_VARIANT,info.value(MSIV_DEFAULT_VARIANT));

    if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
    {
      options.options.remove(MSO_BG_IMAGE_FILE);
      options.options.remove(MSO_BG_IMAGE_LAYOUT);
    }
    options.options.remove(MSO_BG_COLOR);
  }

  return options;
}

void AdiumMessageStylePlugin::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (FSettingsPlugin && FStylePaths.contains(AOptions.styleId))
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    settings->setValueNS(QString(SVN_STYLE_ID).arg(AMessageType),AContext,AOptions.styleId);
    settings->setValueNS(QString(SVN_STYLE_VARIANT).arg(AMessageType),AContext,AOptions.options.value(MSO_VARIANT));
    settings->setValueNS(QString(SVN_STYLE_BG_COLOR).arg(AMessageType),AContext,AOptions.options.value(MSO_BG_COLOR));
    settings->setValueNS(QString(SVN_STYLE_BG_IMAGE_FILE).arg(AMessageType),AContext,AOptions.options.value(MSO_BG_IMAGE_FILE));
    settings->setValueNS(QString(SVN_STYLE_BG_IMAGE_LAYOUT).arg(AMessageType),AContext,AOptions.options.value(MSO_BG_IMAGE_LAYOUT));
  }
  emit styleOptionsChanged(AOptions,AMessageType,AContext);
}

void AdiumMessageStylePlugin::updateAvailStyles()
{
  FStylePaths.clear();
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR) && dir.cd(RSR_STORAGE_ADIUMMESSAGESTYLES))
  {
    QStringList styleDirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    styleDirs.removeAt(styleDirs.indexOf(STORAGE_SHARED_DIR));
    foreach(QString styleDir, styleDirs)
    {
      if (dir.cd(styleDir))
      {
        bool valid = QFile::exists(dir.absoluteFilePath("Contents/Info.plist"));
        valid = valid &&  QFile::exists(dir.absoluteFilePath("Contents/Resources/Incoming/Content.html"));
        if (valid)
        {
          QMap<QString, QVariant> info = AdiumMessageStyle::styleInfo(dir.absolutePath());
          if (info.contains(MSIV_NAME))
            FStylePaths.insert(info.value(MSIV_NAME).toString(),dir.absolutePath());
        }
        dir.cdUp();
      }
    }
  }
}

Q_EXPORT_PLUGIN2(AdiumMessageStylePlugin, AdiumMessageStylePlugin)
