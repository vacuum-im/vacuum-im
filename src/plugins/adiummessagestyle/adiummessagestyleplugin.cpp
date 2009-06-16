#include "adiummessagestyleplugin.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>

#define SVN_STYLE                     "style[]"
#define SVN_STYLE_ID                  SVN_STYLE ":" "styleId"
#define SVN_STYLE_VARIANT             SVN_STYLE ":" "variant"
#define SVN_STYLE_FONT_FAMILY         SVN_STYLE ":" "fontFamily"
#define SVN_STYLE_FONT_SIZE           SVN_STYLE ":" "fontSize"
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

QString AdiumMessageStylePlugin::stylePluginId() const
{
  static QString id = "AdiumMessageStyle";
  return id;
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
      AdiumMessageStyle *style = new AdiumMessageStyle(stylePath,this);
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

IMessageStyleSettings *AdiumMessageStylePlugin::styleSettings(int AMessageType, const QString &AContext, QWidget *AParent)
{
  updateAvailStyles();
  return new AdiumOptionsWidget(this,AMessageType,AContext,AParent);
}

IMessageStyleOptions AdiumMessageStylePlugin::styleOptions(int AMessageType, const QString &AContext) const
{
  IMessageStyleOptions soptions;
  QVariant &styleId = soptions.extended[MSO_STYLE_ID];

  if (FSettingsPlugin)
  {
    QString ns = QString::number(AMessageType)+"|"+AContext;
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    soptions.pluginId = stylePluginId();
    styleId = settings->valueNS(SVN_STYLE_ID,ns);
    soptions.extended.insert(MSO_HEADER_TYPE,AdiumMessageStyle::HeaderNone);
    soptions.extended.insert(MSO_VARIANT,settings->valueNS(SVN_STYLE_VARIANT,ns).toString());
    soptions.extended.insert(MSO_FONT_FAMILY,settings->valueNS(SVN_STYLE_FONT_FAMILY,ns).toString());
    soptions.extended.insert(MSO_FONT_SIZE,settings->valueNS(SVN_STYLE_FONT_SIZE,ns).toInt());
    soptions.extended.insert(MSO_BG_COLOR,settings->valueNS(SVN_STYLE_BG_COLOR,ns).toString());
    soptions.extended.insert(MSO_BG_IMAGE_FILE,settings->valueNS(SVN_STYLE_BG_IMAGE_FILE,ns).toString());
    soptions.extended.insert(MSO_BG_IMAGE_LAYOUT,settings->valueNS(SVN_STYLE_BG_IMAGE_LAYOUT,ns,AdiumMessageStyle::ImageNormal).toInt());
  }

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId.toString()))
  {
    styleId = "yMous";
    switch (AMessageType)
    {
    case Message::GroupChat:
      soptions.extended.insert(MSO_VARIANT,"Saturnine XtraColor Both");
      break;
    default:
      soptions.extended.insert(MSO_VARIANT,"Saturnine Both");
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

void AdiumMessageStylePlugin::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (FSettingsPlugin)
  {
    QString ns = QString::number(AMessageType)+"|"+AContext;
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    if (FStylePaths.contains(AOptions.extended.value(MSO_STYLE_ID).toString()))
    {
      settings->setValueNS(SVN_STYLE_ID,ns,AOptions.extended.value(MSO_STYLE_ID));
      settings->setValueNS(SVN_STYLE_VARIANT,ns,AOptions.extended.value(MSO_VARIANT));
      settings->setValueNS(SVN_STYLE_FONT_FAMILY,ns,AOptions.extended.value(MSO_FONT_FAMILY));
      settings->setValueNS(SVN_STYLE_FONT_SIZE,ns,AOptions.extended.value(MSO_FONT_SIZE));
      settings->setValueNS(SVN_STYLE_BG_COLOR,ns,AOptions.extended.value(MSO_BG_COLOR));
      settings->setValueNS(SVN_STYLE_BG_IMAGE_FILE,ns,AOptions.extended.value(MSO_BG_IMAGE_FILE));
      settings->setValueNS(SVN_STYLE_BG_IMAGE_LAYOUT,ns,AOptions.extended.value(MSO_BG_IMAGE_LAYOUT));
    }
    else
    {
      settings->deleteNS(ns);
    }
  }
  emit styleOptionsChanged(AOptions,AMessageType,AContext);
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
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR) && dir.cd(RSR_STORAGE_ADIUMMESSAGESTYLES))
  {
    QStringList styleDirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    styleDirs.removeAt(styleDirs.indexOf(STORAGE_SHARED_DIR));
    foreach(QString styleDir, styleDirs)
    {
      if (dir.cd(styleDir))
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
        dir.cdUp();
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
      it++;
  }
}

Q_EXPORT_PLUGIN2(AdiumMessageStylePlugin, AdiumMessageStylePlugin)
