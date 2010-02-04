#include "messagestyles.h"

#include <QCoreApplication>

#define SVN_PLUGIN_ID                  "style[]:pluginId"

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
  APluginInfo->name = tr("Message Styles Manager"); 
  APluginInfo->description = tr("Allows to use different styles to display messages");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MessageStyles::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  QList<IPlugin *> plugins = APluginManager->pluginInterface("IMessageStylePlugin");
  foreach (IPlugin *plugin, plugins)
  {
    IMessageStylePlugin *stylePlugin = qobject_cast<IMessageStylePlugin *>(plugin->instance());
    if (stylePlugin)
      FStylePlugins.insert(stylePlugin->stylePluginId(),stylePlugin);
  }

  IPlugin *plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
  if (plugin)
    FAvatars = qobject_cast<IAvatars *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
  if (plugin)
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
  if (plugin)
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
    }
  }

  return !FStylePlugins.isEmpty();
}

bool MessageStyles::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
    FSettingsPlugin->openOptionsNode(ON_MESSAGE_STYLES,tr("Message Styles"),tr("Styles options for custom messages"),MNI_MESSAGE_STYLES,ONO_MESSAGE_STYLES);
  }
  return true;
}

QWidget *MessageStyles::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MESSAGE_STYLES && !FStylePlugins.isEmpty())
  {
    AOrder = OWO_MESSAGE_STYLES;
    StyleOptionsWidget *widget = new StyleOptionsWidget(this);
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),this,SIGNAL(optionsRejected()));
    connect(widget,SIGNAL(optionsAccepted()),this,SIGNAL(optionsAccepted()));
    return widget;
  }
  return NULL;
}

QList<QString> MessageStyles::stylePlugins() const
{
  return FStylePlugins.keys();
}

IMessageStylePlugin *MessageStyles::stylePluginById(const QString &APluginId) const
{
  return FStylePlugins.value(APluginId,NULL);
}

IMessageStyle *MessageStyles::styleForOptions(const IMessageStyleOptions &AOptions) const
{
  IMessageStylePlugin *stylePlugin = stylePluginById(AOptions.pluginId);
  return stylePlugin!=NULL ? stylePlugin->styleForOptions(AOptions) : NULL;
}

IMessageStyleOptions MessageStyles::styleOptions(int AMessageType, const QString &AContext) const
{
  QString pluginId;
  if (FSettingsPlugin)
  {
    QString ns = QString::number(AMessageType)+"|"+AContext;
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    pluginId = settings->valueNS(SVN_PLUGIN_ID,ns).toString();
  }

  if (!FStylePlugins.contains(pluginId))
  {
    pluginId = "Simple";
    if (!FStylePlugins.contains(pluginId) && !FStylePlugins.isEmpty())
      pluginId = FStylePlugins.keys().first();
  }

  IMessageStylePlugin *stylePlugin = stylePluginById(pluginId);
  return stylePlugin!=NULL ? stylePlugin->styleOptions(AMessageType,AContext) : IMessageStyleOptions();
}

void MessageStyles::setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  IMessageStylePlugin *stylePlugin = stylePluginById(AOptions.pluginId);
  if (FSettingsPlugin)
  {
    QString ns = QString::number(AMessageType)+"|"+AContext;
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    if (stylePlugin)
      settings->setValueNS(SVN_PLUGIN_ID,ns,AOptions.pluginId);
    else
      settings->deleteNS(ns);
  }

  if (stylePlugin)
    stylePlugin->setStyleOptions(AOptions,AMessageType,AContext);

  emit styleOptionsChanged(AOptions,AMessageType,AContext);
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

Q_EXPORT_PLUGIN2(plg_messagestyles, MessageStyles)
