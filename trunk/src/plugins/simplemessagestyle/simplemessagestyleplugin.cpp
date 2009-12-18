#include "simplemessagestyleplugin.h"

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

SimpleMessageStylePlugin::SimpleMessageStylePlugin()
{
  FSettingsPlugin = NULL;
}

SimpleMessageStylePlugin::~SimpleMessageStylePlugin()
{

}

void SimpleMessageStylePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Simple Message Style"); 
  APluginInfo->description = tr("Allows to use a simplified style in message design");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
}

bool SimpleMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
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
  static QString id = "Simple";
  return id;
}

QList<QString> SimpleMessageStylePlugin::styles() const
{
  return FStylePaths.keys();
}

IMessageStyle *SimpleMessageStylePlugin::styleForOptions(const IMessageStyleOptions &AOptions)
{
  QString styleId = AOptions.extended.value(MSO_STYLE_ID).toString();
  if (!FStyles.contains(styleId))
  {
    QString stylePath = FStylePaths.value(styleId);
    if (!stylePath.isEmpty())
    {
      SimpleMessageStyle *style = new SimpleMessageStyle(stylePath,this);
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

IMessageStyleSettings *SimpleMessageStylePlugin::styleSettings(int AMessageType, const QString &AContext, QWidget *AParent)
{
  updateAvailStyles();
  return new SimpleOptionsWidget(this,AMessageType,AContext,AParent);
}

IMessageStyleOptions SimpleMessageStylePlugin::styleOptions(int AMessageType, const QString &AContext) const
{
  IMessageStyleOptions soptions;
  QVariant &styleId = soptions.extended[MSO_STYLE_ID];

  if (FSettingsPlugin)
  {
    QString ns = QString::number(AMessageType)+"|"+AContext;
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    soptions.pluginId = stylePluginId();
    styleId = settings->valueNS(SVN_STYLE_ID,ns);
    soptions.extended.insert(MSO_VARIANT,settings->valueNS(SVN_STYLE_VARIANT,ns).toString());
    soptions.extended.insert(MSO_FONT_FAMILY,settings->valueNS(SVN_STYLE_FONT_FAMILY,ns).toString());
    soptions.extended.insert(MSO_FONT_SIZE,settings->valueNS(SVN_STYLE_FONT_SIZE,ns).toInt());
    soptions.extended.insert(MSO_BG_COLOR,settings->valueNS(SVN_STYLE_BG_COLOR,ns).toString());
    soptions.extended.insert(MSO_BG_IMAGE_FILE,settings->valueNS(SVN_STYLE_BG_IMAGE_FILE,ns).toString());
  }

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId.toString()))
  {
    switch (AMessageType)
    {
    case Message::Normal:
    case Message::Headline:
    case Message::Error:
      styleId = "Style for messages";
      break;
    case Message::GroupChat:
      styleId = "Style for conferences";
      break;
    default:
      styleId = "Style for chats";
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

void SimpleMessageStylePlugin::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
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
    }
    else
    {
      settings->deleteNS(ns);
    }
  }
  emit styleOptionsChanged(AOptions,AMessageType,AContext);
}

QList<QString> SimpleMessageStylePlugin::styleVariants(const QString &AStyleId) const
{
  if (FStyles.contains(AStyleId))
    return FStyles.value(AStyleId)->variants();
  return SimpleMessageStyle::styleVariants(FStylePaths.value(AStyleId));
}

QMap<QString,QVariant> SimpleMessageStylePlugin::styleInfo(const QString &AStyleId) const
{
  if (FStyles.contains(AStyleId))
    return FStyles.value(AStyleId)->infoValues();
  return SimpleMessageStyle::styleInfo(FStylePaths.value(AStyleId));
}

void SimpleMessageStylePlugin::updateAvailStyles()
{
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(RESOURCES_DIR) && dir.cd(RSR_STORAGE_SIMPLEMESSAGESTYLES))
  {
    QStringList styleDirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    styleDirs.removeAt(styleDirs.indexOf(STORAGE_SHARED_DIR));
    foreach(QString styleDir, styleDirs)
    {
      if (dir.cd(styleDir))
      {
        if (!FStylePaths.values().contains(dir.absolutePath()))
        {
          bool valid = QFile::exists(dir.absoluteFilePath("Info.plist"));
          valid = valid &&  QFile::exists(dir.absoluteFilePath("Incoming/Content.html"));
          if (valid)
          {
            QMap<QString, QVariant> info = SimpleMessageStyle::styleInfo(dir.absolutePath());
            if (!info.value(MSIV_NAME).toString().isEmpty())
              FStylePaths.insert(info.value(MSIV_NAME).toString(),dir.absolutePath());
          }
        }
        dir.cdUp();
      }
    }
  }
}

void SimpleMessageStylePlugin::onStyleWidgetAdded(QWidget *AWidget)
{
  SimpleMessageStyle *style = qobject_cast<SimpleMessageStyle *>(sender());
  if (style)
    emit styleWidgetAdded(style,AWidget);
}

void SimpleMessageStylePlugin::onStyleWidgetRemoved(QWidget *AWidget)
{
  SimpleMessageStyle *style = qobject_cast<SimpleMessageStyle *>(sender());
  if (style)
  {
    if (style->styleWidgets().isEmpty())
      QTimer::singleShot(0,this,SLOT(onClearEmptyStyles()));
    emit styleWidgetRemoved(style,AWidget);
  }
}

void SimpleMessageStylePlugin::onClearEmptyStyles()
{
  QMap<QString, SimpleMessageStyle *>::iterator it = FStyles.begin();
  while (it!=FStyles.end())
  {
    SimpleMessageStyle *style = it.value();
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

Q_EXPORT_PLUGIN2(plg_simplemessagestyle, SimpleMessageStylePlugin)
