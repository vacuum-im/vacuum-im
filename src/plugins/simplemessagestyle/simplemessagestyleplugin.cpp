#include "simplemessagestyleplugin.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>

SimpleMessageStylePlugin::SimpleMessageStylePlugin()
{

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
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool SimpleMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  Q_UNUSED(APluginManager);
  Q_UNUSED(AInitOrder);
  return true;
}

bool SimpleMessageStylePlugin::initObjects()
{
  updateAvailStyles();
  return true;
}

QString SimpleMessageStylePlugin::pluginId() const
{
  static QString id = "SimpleMessageStyle";
  return id;
}

QString SimpleMessageStylePlugin::pluginName() const
{
  return tr("Simple Style");
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

IMessageStyleOptions SimpleMessageStylePlugin::styleOptions(const OptionsNode &ANode, int AMessageType) const
{
  IMessageStyleOptions soptions;
  QVariant &styleId = soptions.extended[MSO_STYLE_ID];

  soptions.pluginId = pluginId();
  styleId = ANode.value("style-id").toString();
  soptions.extended.insert(MSO_VARIANT,ANode.value("variant"));
  soptions.extended.insert(MSO_FONT_FAMILY,ANode.value("font-family"));
  soptions.extended.insert(MSO_FONT_SIZE,ANode.value("font-size"));
  soptions.extended.insert(MSO_BG_COLOR,ANode.value("bg-color"));
  soptions.extended.insert(MSO_BG_IMAGE_FILE,ANode.value("bg-image-file"));

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(styleId.toString()))
  {
    switch (AMessageType)
    {
    case Message::Normal:
    case Message::Headline:
    case Message::Error:
      styleId = "Message Style";
      break;
    default:
      styleId = "Chat Style";
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

IOptionsWidget *SimpleMessageStylePlugin::styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent)
{
  updateAvailStyles();
  return new SimpleOptionsWidget(this,ANode,AMessageType,AParent);
}

void SimpleMessageStylePlugin::saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
  SimpleOptionsWidget *widget = qobject_cast<SimpleOptionsWidget *>(AWidget->instance());
  if (widget)
    widget->apply(ANode);
}

void SimpleMessageStylePlugin::saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions)
{
  SimpleOptionsWidget *widget = qobject_cast<SimpleOptionsWidget *>(AWidget->instance());
  if (widget)
    AOptions = widget->styleOptions();
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
  foreach(QString substorage, FileStorage::availSubStorages(RSR_STORAGE_SIMPLEMESSAGESTYLES, false))
  {
    QDir dir(FileStorage::subStorageDir(RSR_STORAGE_SIMPLEMESSAGESTYLES,substorage));
    if (dir.exists())
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
