#include "messagestyles.h"

#include <QCoreApplication>

#define SVN_PLUGIN_ID                  "style%1[]:pluginId"

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
  QList<IPlugin *> plugins = APluginManager->getPlugins("IMessageStylePlugin");
  foreach (IPlugin *plugin, plugins)
  {
    IMessageStylePlugin *stylePlugin = qobject_cast<IMessageStylePlugin *>(plugin->instance());
    if (stylePlugin)
    {
      FStylePlugins.insert(stylePlugin->stylePluginId(),stylePlugin);
    }
  }

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

QList<QString> MessageStyles::stylePlugins() const
{
  return FStylePlugins.keys();
}

IMessageStylePlugin *MessageStyles::stylePluginById(const QString &APluginId) const
{
  return FStylePlugins.value(APluginId,NULL);
}

IMessageStyle *MessageStyles::styleById(const QString &APluginId, const QString &AStyleId) const
{
  IMessageStylePlugin *stylePlugin = stylePluginById(APluginId);
  return stylePlugin!=NULL ? stylePlugin->styleById(AStyleId) : NULL;
}

IMessageStyleOptions MessageStyles::styleOptions(int AMessageType, const QString &AContext) const
{
  QString pluginId;
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    pluginId = settings->valueNS(QString(SVN_PLUGIN_ID).arg(AMessageType),AContext).toString();
  }

  if (!FStylePlugins.contains(pluginId))
  {
    pluginId = "SimpleMessageStyle";
    if (!FStylePlugins.contains(pluginId) && !FStylePlugins.isEmpty())
      pluginId = FStylePlugins.keys().first();
  }

  IMessageStylePlugin *stylePlugin = stylePluginById(pluginId);
  return stylePlugin!=NULL ? stylePlugin->styleOptions(AMessageType,AContext) : IMessageStyleOptions();
}

void MessageStyles::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  IMessageStylePlugin *stylePlugin = stylePluginById(AOptions.pluginId);
  if (stylePlugin)
  {
    stylePlugin->setStyleOptions(AOptions,AMessageType,AContext);
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

QString MessageStyles::timeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime) const
{
  int daysDelta = AMessageTime.daysTo(ACurTime);
  if (daysDelta > 365)
    return tr("d MMM yyyy hh:mm");
  else if (daysDelta > 0)
    return tr("d MMM hh:mm");
  return tr("hh:mm:ss");
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
