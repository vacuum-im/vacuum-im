#include "infowidget.h"

#include <QMovie>
#include <QDataStream>

InfoWidget::InfoWidget(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);
  ui.lblAvatar->setVisible(false);
  ui.wdtAccount->setVisible(false);
  ui.wdtStatus->setVisible(false);
  ui.wdtEmail->setVisible(false);
  ui.wdtClient->setVisible(false); 
  ui.lblStatus->setMaximumHeight(QFontMetrics(ui.lblStatus->font()).height()*4+1);

  FAccount = NULL;
  FRoster = NULL;
  FPresence = NULL;
  FClientInfo = NULL;
  FAvatars = NULL;

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FAutoFields = 0xFFFFFFFF;
  FVisibleFields = AccountName|ContactAvatar|ContactName|ContactStatus;

  initialize();
}

InfoWidget::~InfoWidget()
{

}

void InfoWidget::autoUpdateFields()
{
  if (isFiledAutoUpdated(AccountName))
    autoUpdateField(AccountName);
  if (isFiledAutoUpdated(ContactAvatar))
    autoUpdateField(ContactAvatar);
  if (isFiledAutoUpdated(ContactName))
    autoUpdateField(ContactName);
  if (isFiledAutoUpdated(ContactShow))
    autoUpdateField(ContactShow);
  if (isFiledAutoUpdated(ContactStatus))
    autoUpdateField(ContactStatus);
  if (isFiledAutoUpdated(ContactEmail))
    autoUpdateField(ContactEmail);
  if (isFiledAutoUpdated(ContactClient))
    autoUpdateField(ContactClient);
}

void InfoWidget::autoUpdateField(InfoField AField)
{
  switch(AField)
  {
  case AccountName:
    {
      FAccount != NULL ? setField(AccountName,FAccount->name()) : setField(AccountName,QVariant());
      break;
    };
  case ContactAvatar:
    {
      QImage avatar = FAvatars!=NULL ? FAvatars->avatarImage(FContactJid) : QImage();
      setField(ContactAvatar,avatar);
      break;
    };
  case ContactName:
    {
      QString contactName;
      if (!(FStreamJid && FContactJid))
        contactName = FRoster ? FRoster->rosterItem(FContactJid).name : FContactJid.node();
      else
        contactName = FContactJid.resource();
      if (contactName.isEmpty())
        contactName = FContactJid.bare();
      setField(ContactName,contactName);
      break;
    };
  case ContactShow:
    {
      if (FPresence)
      {
        setField(ContactShow,FPresence->presenceItem(FContactJid).show);
      }
      break;
    }
  case ContactStatus:
    {
      if (FPresence)
      {
        setField(ContactStatus,FPresence->presenceItem(FContactJid).status);
      }
      break;
    };
  case ContactEmail:
    {
      setField(ContactEmail,QVariant());
      break;
    };
  case ContactClient:
    {
      if (FClientInfo && FClientInfo->hasSoftwareInfo(FContactJid))
        setField(ContactClient,FClientInfo->softwareName(FContactJid)+" "+FClientInfo->softwareVersion(FContactJid));
      else
        setField(ContactClient,QVariant());
      break;
    };
  }
}

QVariant InfoWidget::field(InfoField AField) const
{
  switch(AField)
  {
  case AccountName:         return ui.lblAccount->text();
  case ContactAvatar:       return ui.lblAvatar->picture();
  case ContactName:         return FContactName;
  case ContactShow:         return FContactShow;
  case ContactStatus:       return ui.lblStatus->text();
  case ContactEmail:        return ui.lblEmail->text();
  case ContactClient:       return ui.lblClient->text();
  }
  return QVariant();
}

void InfoWidget::setField(InfoField AField, const QVariant &AValue)
{
  switch(AField)
  {
  case AccountName:
    {
      ui.lblAccount->setText(Qt::escape(AValue.toString()));
      ui.wdtAccount->setVisible(isFieldVisible(AccountName) && !AValue.toString().isEmpty());
      break;
    };
  case ContactAvatar:
    {
      QImage avatar = qvariant_cast<QImage>(AValue);
      if (!avatar.isNull())
        ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
      else
        ui.lblAvatar->clear();
      ui.lblAvatar->setVisible(isFieldVisible(ContactAvatar) && !avatar.isNull());
      break;
    };
  case ContactName:
    {
      FContactName = AValue.toString();
      ui.lblName->setText(QString("<b>%1</b> (%2)").arg(Qt::escape(FContactName)).arg(FContactJid.hFull()));
      ui.lblName->setVisible(isFieldVisible(ContactName));
      break;
    };
  case ContactShow:
    {
      FContactShow = AValue.toInt();
      break;
    }
  case ContactStatus:
    {
      ui.lblStatus->setText(Qt::escape(AValue.toString()));
      ui.wdtStatus->setVisible(isFieldVisible(ContactStatus) && !AValue.toString().isEmpty());
      break;
    };
  case ContactEmail:
    {
      ui.lblEmail->setText(Qt::escape(AValue.toString()));
      ui.wdtEmail->setVisible(isFieldVisible(ContactEmail) && !AValue.toString().isEmpty());
      break;
    };
  case ContactClient:
    {
      ui.lblClient->setText(Qt::escape(AValue.toString()));
      ui.wdtClient->setVisible(isFieldVisible(ContactClient) && !AValue.toString().isEmpty());
      break;
    };
  }
  emit fieldChanged(AField,AValue);
}

bool InfoWidget::isFiledAutoUpdated(IInfoWidget::InfoField AField) const
{
  return (FAutoFields & AField) > 0;
}

void InfoWidget::setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto)
{
  if (isFiledAutoUpdated(AField) != AAuto)
  {
    if (AAuto)
    {
      FAutoFields |= AField;
      autoUpdateField(AField);
    }
    else
      FAutoFields &= ~AField;
  }
}

bool InfoWidget::isFieldVisible(IInfoWidget::InfoField AField) const
{
  return (FVisibleFields & AField) >0;
}

void InfoWidget::setFieldVisible(IInfoWidget::InfoField AField, bool AVisible)
{
  if (isFieldVisible(AField) != AVisible)
  {
    AVisible ? FVisibleFields |= AField : FVisibleFields &= ~AField;
    setField(AField,field(AField));
  }
}

void InfoWidget::setStreamJid(const Jid &AStreamJid)
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  initialize();
  autoUpdateFields();
  emit streamJidChanged(befour);
}

void InfoWidget::setContactJid(const Jid &AContactJid)
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  autoUpdateFields();
  emit contactJidChanged(AContactJid);
}

void InfoWidget::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    IAccountManager *accountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (accountManager)
    {
      if (FAccount)
      {
        disconnect(FAccount->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          this, SLOT(onAccountChanged(const QString &, const QVariant &)));
      }

      FAccount = accountManager->accountByStream(FStreamJid);
      if (FAccount)
      {
        connect(FAccount->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          SLOT(onAccountChanged(const QString &, const QVariant &)));
      }
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (rosterPlugin)
    {
      if (FRoster)
      {
        disconnect(FRoster->instance(),SIGNAL(received(const IRosterItem &)),
          this, SLOT(onRosterItemReceived(const IRosterItem &)));
      }

      FRoster = rosterPlugin->getRoster(FStreamJid);
      if (FRoster)
      {
        connect(FRoster->instance(),SIGNAL(received(const IRosterItem &)),
          SLOT(onRosterItemReceived(const IRosterItem &)));
      }
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (presencePlugin)
    {
      if (FPresence)
      {
        disconnect(FPresence->instance(),SIGNAL(received(const IPresenceItem &)),
          this, SLOT(onPresenceReceived(const IPresenceItem &)));
      }

      FPresence = presencePlugin->getPresence(FStreamJid);
      if (FPresence)
      {
        connect(FPresence->instance(),SIGNAL(received(const IPresenceItem &)),
          SLOT(onPresenceReceived(const IPresenceItem &)));
      }
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IClientInfo").value(0,NULL);
  if (plugin)
  {
    FClientInfo = qobject_cast<IClientInfo *>(plugin->instance());
    if (FClientInfo)
    {
      connect(FClientInfo->instance(),SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onSoftwareInfoChanged(const Jid &)));
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IAvatars").value(0,NULL);
  if (plugin)
  {
    FAvatars = qobject_cast<IAvatars *>(plugin->instance());
    if (FAvatars)
    {
      connect(FAvatars->instance(),SIGNAL(avatarChanged(const Jid &)),SLOT(onAvatarChanged(const Jid &)));
    }
  }
}

QString InfoWidget::showName(IPresence::Show AShow) const
{
  switch(AShow)
  {
  case IPresence::Online:
    return tr("Online");
  case IPresence::Chat:
    return tr("Chat");
  case IPresence::Away:
    return tr("Away");
  case IPresence::ExtendedAway:
    return tr("Extended Away");
  case IPresence::DoNotDisturb:
    return tr("Do not Disturb");
  case IPresence::Invisible:
    return tr("Invisible");
  case IPresence::Error:
    return tr("Error");
  case IPresence::Offline:
    return tr("Offline");
  default:
    return tr("Unknown status");
  }
}

void InfoWidget::onAccountChanged(const QString &AName, const QVariant &/*AValue*/)
{
  if (isFiledAutoUpdated(AccountName) && AName == AVN_NAME)
    autoUpdateField(AccountName);
}

void InfoWidget::onRosterItemReceived(const IRosterItem &ARosterItem)
{
  if (isFiledAutoUpdated(ContactName) && (ARosterItem.itemJid && FContactJid))
    autoUpdateField(ContactName);
}

void InfoWidget::onPresenceReceived(const IPresenceItem &APresenceItem)
{
  if (APresenceItem.itemJid == FContactJid)
  {
    if (isFiledAutoUpdated(ContactShow))
      setField(ContactShow,APresenceItem.show);
    if (isFiledAutoUpdated(ContactStatus))
      setField(ContactStatus,APresenceItem.status);
  }
}

void InfoWidget::onSoftwareInfoChanged(const Jid &AContactJid)
{
  if (isFiledAutoUpdated(ContactClient) && AContactJid == FContactJid)
    autoUpdateField(ContactClient);
}

void InfoWidget::onAvatarChanged(const Jid &AContactJid)
{
  if (isFiledAutoUpdated(ContactAvatar) && (FContactJid && AContactJid))
    autoUpdateField(ContactAvatar);
}
