#include "messagestyles.h"

#include <QCoreApplication>

#define SVN_SETTINGS                    "styleSettings[]"
#define SVN_SETTINGS_STYLEID            SVN_SETTINGS ":" "styleId"
#define SVN_SETTINGS_VARIANT            SVN_SETTINGS ":" "variant"
#define SVN_SETTINGS_AVATARS            SVN_SETTINGS ":" "showAvarats"
#define SVN_SETTINGS_STATUS_ICONS       SVN_SETTINGS ":" "showStatusIcons"
#define SVN_SETTINGS_BG_COLOR           SVN_SETTINGS ":" "bgColor"
#define SVN_SETTINGS_BG_IMAGE_FILE      SVN_SETTINGS ":" "bgImageFile"
#define SVN_SETTINGS_BG_IMAGE_LAYOUT    SVN_SETTINGS ":" "bgImageLayout"

MessageStyles::MessageStyles()
{
  FSettingsPlugin = NULL;
  FAvatars = NULL;
  FStatusIcons = NULL;
  FVCardPlugin = NULL;
  FRosterPlugin = NULL;
}

MessageStyles::~MessageStyles()
{

}

void MessageStyles::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing message styles");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Message Styles"); 
  APluginInfo->uid = MESSAGESTYLES_UUID;
  APluginInfo->version = "0.1";
}

bool MessageStyles::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IAvatars").value(0,NULL);
  if (plugin)
    FAvatars = qobject_cast<IAvatars *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
    }
  }

  return true;
}

bool MessageStyles::initObjects()
{
  updateAvailStyles();
  return true;
}

IMessageStyle *MessageStyles::styleById(const QString &AStyleId)
{
  if (!FStyles.contains(AStyleId))
  {
    QString stylePath = FStylePaths.value(AStyleId);
    if (!stylePath.isEmpty())
    {
      IMessageStyle *style = new MessageStyle(stylePath,this);
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

QList<QString> MessageStyles::styles() const
{
  return FStylePaths.values();
}

QList<QString> MessageStyles::styleVariants(const QString &AStyleId) const
{
  if (FStyles.contains(AStyleId))
    return FStyles.value(AStyleId)->variants();
  return MessageStyle::styleVariants(FStylePaths.value(AStyleId));
}

QMap<QString, QVariant> MessageStyles::styleInfo(const QString &AStyleId) const
{
  if (FStyles.contains(AStyleId))
    return FStyles.value(AStyleId)->infoValues();
  return MessageStyle::styleInfo(FStylePaths.value(AStyleId));
}

IMessageStyles::StyleSettings MessageStyles::styleSettings(int AMessageType) const
{
  IMessageStyles::StyleSettings sSettings;
  
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    QString ns = QString::number(AMessageType);
    sSettings.styleId = settings->valueNS(SVN_SETTINGS_STYLEID,ns).toString();
    sSettings.variant = settings->valueNS(SVN_SETTINGS_VARIANT,ns).toString();
    sSettings.bgColor = settings->valueNS(SVN_SETTINGS_BG_COLOR,ns).toString();
    sSettings.bgImageFile = settings->valueNS(SVN_SETTINGS_BG_IMAGE_FILE,ns).toString();
    sSettings.bgImageLayout = settings->valueNS(SVN_SETTINGS_BG_IMAGE_LAYOUT,ns,0).toInt();
    sSettings.content.showAvatars = settings->valueNS(SVN_SETTINGS_AVATARS,ns,true).toBool();
    sSettings.content.showStatusIcons = settings->valueNS(SVN_SETTINGS_STATUS_ICONS,ns,true).toBool();
  }

  if (!FStylePaths.isEmpty() && !FStylePaths.contains(sSettings.styleId))
  {
    sSettings.styleId = "yMous";
    switch (AMessageType)
    {
    case Message::GroupChat:
      sSettings.variant = "Saturnine XtraColor Both";
      break;
    default:
      sSettings.variant = "Saturnine Both";
    }
    
    if (!FStylePaths.contains(sSettings.styleId))  
      sSettings.styleId = FStylePaths.keys().first();

    QMap<QString,QVariant> info = styleInfo(sSettings.styleId);
    if (!styleVariants(sSettings.styleId).contains(sSettings.variant))
      sSettings.variant = info.value(MSIV_DEFAULT_VARIANT).toString();
    sSettings.bgImageLayout = IMessageStyle::ImageNormal;
    if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
      sSettings.bgImageFile.clear();
    sSettings.bgColor.clear();
    sSettings.content.showStatusIcons = true;
    sSettings.content.showAvatars = info.value(MSIV_SHOW_USER_ICONS,true).toBool();
  }

  return sSettings;
}

void MessageStyles::setStyleSettings(int AMessageType, const IMessageStyles::StyleSettings &ASettings)
{
  if (FSettingsPlugin && FStylePaths.contains(ASettings.styleId))
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    QString ns = QString::number(AMessageType);
    settings->setValueNS(SVN_SETTINGS_STYLEID,ns,ASettings.styleId);
    settings->setValueNS(SVN_SETTINGS_VARIANT,ns,ASettings.variant);
    settings->setValueNS(SVN_SETTINGS_BG_COLOR,ns,ASettings.bgColor);
    settings->setValueNS(SVN_SETTINGS_BG_IMAGE_FILE,ns,ASettings.bgImageFile);
    settings->setValueNS(SVN_SETTINGS_BG_IMAGE_LAYOUT,ns,ASettings.bgImageLayout);
    settings->setValueNS(SVN_SETTINGS_AVATARS,ns,ASettings.content.showAvatars);
    settings->setValueNS(SVN_SETTINGS_STATUS_ICONS,ns,ASettings.content.showStatusIcons);
  }
}

QString MessageStyles::userAvatar(const Jid &AContactJid) const
{
  return FAvatars!=NULL ? FAvatars->avatarFileName(FAvatars->avatarHash(AContactJid)) : QString::null;
}

QString MessageStyles::userName(const Jid &AStreamJid, const Jid &AContactJid) const
{
  QString name;
  if (!AContactJid.isValid())
  {
    if (!FStreamNames.contains(AStreamJid.bare()))
    {
      IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AStreamJid.bare()) : NULL;
      if (vcard!=NULL)
      {
        name = vcard->value(VVN_NICKNAME);
        vcard->unlock();
      }
      FStreamNames.insert(AStreamJid.bare(),name);
    }
    else
      name = FStreamNames.value(AStreamJid.bare());
  }
  else if (AStreamJid && AContactJid)
  {
    name = !AContactJid.resource().isEmpty() ? AContactJid.resource() : AContactJid.node();
  }
  else
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
    name = roster!=NULL ? roster->rosterItem(AContactJid).name : QString::null;
  }

  if (name.isEmpty())
  {
    if (AContactJid.isValid())
      name = !AContactJid.node().isEmpty() ? AContactJid.node() : AContactJid.domain();
    else
      name = !AStreamJid.node().isEmpty() ? AStreamJid.node() : AStreamJid.domain();
  }

  return name;
}

QString MessageStyles::userIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (FStatusIcons)
  {
    QString iconKey;
    if (AContactJid.isValid())
      iconKey = FStatusIcons->iconKeyByJid(AStreamJid,AContactJid);
    else
      iconKey = FStatusIcons->iconKeyByStatus(IPresence::Online,SUBSCRIPTION_BOTH,false);
    QString substorage = FStatusIcons->subStorageByJid(AContactJid.isValid() ? AContactJid : AStreamJid);
    return FStatusIcons->iconFileName(substorage,iconKey);
  }
  return QString::null;
}

QString MessageStyles::userIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const
{
  if (FStatusIcons)
  {
    QString iconKey = FStatusIcons->iconKeyByStatus(AShow,ASubscription,AAsk);
    QString substorage = FStatusIcons->subStorageByJid(AContactJid);
    return FStatusIcons->iconFileName(substorage,iconKey);
  }
  return QString::null;
}

QString MessageStyles::messageTimeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime) const
{
  int daysDelta = AMessageTime.daysTo(ACurTime);
  if (daysDelta > 365)
    return tr("d MMM yyyy hh:mm");
  else if (daysDelta > 0)
    return tr("d MMM hh:mm");
  return tr("hh:mm:ss");
}

void MessageStyles::updateAvailStyles()
{
  FStylePaths.clear();
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR) && dir.cd(RSR_STORAGE_MESSAGESTYLES))
  {
    QStringList styleDirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    styleDirs.removeAt(styleDirs.indexOf(STORAGE_SHARED_DIR));
    foreach(QString styleDir, styleDirs)
    {
      if (dir.cd(styleDir) && QFile::exists(dir.absoluteFilePath("Contents/Resources/Incoming/Content.html")))
      {
        QMap<QString, QVariant> info = MessageStyle::styleInfo(dir.absolutePath());
        if (info.contains(MSIV_NAME))
          FStylePaths.insert(info.value(MSIV_NAME).toString(),dir.absolutePath());
        dir.cdUp();
      }
    }
  }
}

void MessageStyles::onVCardChanged(const Jid &AContactJid)
{
  if (FStreamNames.contains(AContactJid.bare()))
  {
    IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AContactJid) : NULL;
    if (vcard!=NULL)
    {
      FStreamNames.insert(AContactJid.bare(),vcard->value(VVN_NICKNAME));
      vcard->unlock();
    }
  }
}

Q_EXPORT_PLUGIN2(MessageStylesPlugin, MessageStyles)
