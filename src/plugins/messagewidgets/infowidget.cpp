#include "infowidget.h"

#include <QMovie>
#include <QDataStream>
#include <QImageReader>

InfoWidget::InfoWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);
  ui.lblAvatar->setVisible(false);
  ui.wdtAccount->setVisible(false);
  ui.wdtStatus->setVisible(false);
  ui.lblStatus->setMaximumHeight(ui.lblStatus->fontMetrics().height()*4+1);

  FAccount = NULL;
  FRoster = NULL;
  FPresence = NULL;
  FAvatars = NULL;

  FMessageWidgets = AMessageWidgets;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FAutoFields = 0xFFFFFFFF;
  FVisibleFields = AccountName|ContactName|ContactStatus|ContactAvatar;

  initialize();
}

InfoWidget::~InfoWidget()
{

}

void InfoWidget::setStreamJid(const Jid &AStreamJid)
{
  if (FStreamJid != AStreamJid)
  {
    Jid befour = FStreamJid;
    FStreamJid = AStreamJid;
    initialize();
    autoUpdateFields();
    emit streamJidChanged(befour);
  }
}

void InfoWidget::setContactJid(const Jid &AContactJid)
{
  if (FContactJid != AContactJid)
  {
    Jid befour = FContactJid;
    FContactJid = AContactJid;
    autoUpdateFields();
    emit contactJidChanged(AContactJid);
  }
}

void InfoWidget::autoUpdateFields()
{
  if (isFiledAutoUpdated(AccountName))
    autoUpdateField(AccountName);
  if (isFiledAutoUpdated(ContactName))
    autoUpdateField(ContactName);
  if (isFiledAutoUpdated(ContactShow))
    autoUpdateField(ContactShow);
  if (isFiledAutoUpdated(ContactStatus))
    autoUpdateField(ContactStatus);
  if (isFiledAutoUpdated(ContactAvatar))
    autoUpdateField(ContactAvatar);
}

void InfoWidget::autoUpdateField(InfoField AField)
{
  switch(AField)
  {
  case AccountName:
    {
      setField(AField,FAccount!=NULL ? FAccount->name() : QString());
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
      setField(AField,contactName);
      break;
    };
  case ContactShow:
    {
      setField(AField,FPresence!=NULL ? FPresence->presenceItem(FContactJid).show : IPresence::Offline);
      break;
    }
  case ContactStatus:
    {
      setField(AField,FPresence!=NULL ? FPresence->presenceItem(FContactJid).status : QString());
      break;
    };
  case ContactAvatar:
    {
      setField(AField, FAvatars!=NULL ? FAvatars->avatarFileName(FAvatars->avatarHash(FContactJid)) : QString());
      break;
    };
  }
}

QVariant InfoWidget::field(InfoField AField) const
{
  return FFieldValues.value(AField);
}

void InfoWidget::setField(InfoField AField, const QVariant &AValue)
{
  FFieldValues.insert(AField,AValue);
  updateFieldLabel(AField);
  emit fieldChanged(AField,AValue);
}

int InfoWidget::autoUpdatedFields() const
{
  return FAutoFields;
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

int InfoWidget::visibleFields() const
{
  return FVisibleFields;
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
    updateFieldLabel(AField);
  }
}

void InfoWidget::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IAccountManager").value(0,NULL);
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

  plugin = FMessageWidgets->pluginManager()->pluginInterface("IRosterPlugin").value(0,NULL);
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

  plugin = FMessageWidgets->pluginManager()->pluginInterface("IPresencePlugin").value(0,NULL);
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

  plugin = FMessageWidgets->pluginManager()->pluginInterface("IAvatars").value(0,NULL);
  if (plugin)
  {
    FAvatars = qobject_cast<IAvatars *>(plugin->instance());
    if (FAvatars)
    {
      connect(FAvatars->instance(),SIGNAL(avatarChanged(const Jid &)),SLOT(onAvatarChanged(const Jid &)));
    }
  }
}

void InfoWidget::updateFieldLabel(IInfoWidget::InfoField AField)
{
  switch(AField)
  {
  case AccountName:
    {
      QString name = field(AField).toString();
      ui.lblAccount->setText(Qt::escape(name));
      ui.wdtAccount->setVisible(isFieldVisible(AField) && !name.isEmpty());
      break;
    };
  case ContactName:
    {
      QString name = field(AField).toString();
      ui.lblName->setText(QString("<b>%1</b> (%2)").arg(Qt::escape(name)).arg(FContactJid.hFull()));
      ui.lblName->setVisible(isFieldVisible(AField));
      break;
    };
  case ContactStatus:
    {
      QString status = field(AField).toString();
      ui.lblStatus->setText(Qt::escape(status));
      ui.wdtStatus->setVisible(isFieldVisible(AField) && !status.isEmpty());
      break;
    };
  case ContactAvatar:
    {
      if (ui.lblAvatar->movie()!=NULL)
        ui.lblAvatar->movie()->deleteLater();
      
      QString fileName = field(AField).toString();
      if (!fileName.isEmpty())
      {
        QMovie *movie = new QMovie(fileName,QByteArray(),ui.lblAvatar);
        QSize size = QImageReader(fileName).size();
        size.scale(QSize(32,32),Qt::KeepAspectRatio);
        movie->setScaledSize(size);
        ui.lblAvatar->setMovie(movie);
        movie->start();
      }
      else
      {
        ui.lblAvatar->setMovie(NULL);
      }
      ui.lblAvatar->setVisible(isFieldVisible(AField) && !fileName.isEmpty());

      break;
    };
  default:
    break;
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

void InfoWidget::onAvatarChanged(const Jid &AContactJid)
{
  if (isFiledAutoUpdated(ContactAvatar) && (FContactJid && AContactJid))
    autoUpdateField(ContactAvatar);
}
