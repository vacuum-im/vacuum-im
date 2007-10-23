#include "infowidget.h"

#include <QMovie>
#include <QDataStream>

InfoWidget::InfoWidget(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FAccount = NULL;
  FRoster = NULL;
  FPresence = NULL;

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  initialize();
}

InfoWidget::~InfoWidget()
{

}

void InfoWidget::autoSetFields()
{
  autoSetField(AccountName);
  autoSetField(ContactAvatar);
  autoSetField(ContactName);
  autoSetField(ContactShow);
  autoSetField(ContactStatus);
  autoSetField(ContactEmail);
  autoSetField(ContactClient);
}

void InfoWidget::autoSetField(InfoField AField)
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
      setField(ContactAvatar,QVariant());
      break;
    };
  case ContactName:
    {
      QString contactName;
      if (!FStreamJid.equals(FContactJid,false))
      {
        contactName = FContactJid.node();
        if (FRoster)
        {
          IRosterItem *rosterItem = FRoster->item(FContactJid);
          if (rosterItem)
            contactName = rosterItem->name();
        }
      }
      else
        contactName = FContactJid.resource();
      setField(ContactName,contactName);
      break;
    };
  case ContactShow:
    {
      if (FPresence)
      {
        IPresenceItem *presenceItem = FPresence->item(FContactJid);
        presenceItem != NULL ? setField(ContactShow,showName(presenceItem->show())) : setField(ContactShow,showName(IPresence::Offline));
      }
      break;
    }
  case ContactStatus:
    {
      if (FPresence)
      {
        IPresenceItem *presenceItem = FPresence->item(FContactJid);
        presenceItem != NULL ? setField(ContactStatus,presenceItem->status()) : setField(ContactStatus,QVariant());
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
      ui.lblAccount->setText(AValue.toString());
      ui.wdtAccount->setVisible(!AValue.toString().isEmpty());
      break;
    };
  case ContactAvatar:
    {
      QByteArray data = AValue.toByteArray();
      if (!data.isEmpty())
      {
        QDataStream dataStream(data);
        QMovie *oldMovie = ui.lblAvatar->movie();
        QMovie *movie = new QMovie(dataStream.device(),QByteArray(),ui.lblAvatar);
        movie->start();
        ui.lblAvatar->setMovie(movie);
        ui.lblAvatar->setVisible(true);
        delete oldMovie;
      }
      else
        ui.lblAvatar->setVisible(false);
      break;
    };
  case ContactName:
    {
      FContactName = AValue.toString();
      ui.lblName->setText(QString("<b>%1</b> (%2)").arg(FContactName).arg(FContactJid.full()));
      break;
    };
  case ContactShow:
    {
      FContactShow = AValue.toString();
      break;
    }
  case ContactStatus:
    {
      ui.lblStatus->setText(AValue.toString());
      ui.wdtStatus->setVisible(!AValue.toString().isEmpty());
      break;
    };
  case ContactEmail:
    {
      ui.lblEmail->setText(AValue.toString());
      ui.wdtEmail->setVisible(!AValue.toString().isEmpty());
      break;
    };
  case ContactClient:
    {
      ui.lblClient->setText(AValue.toString());
      ui.wdtClient->setVisible(!AValue.toString().isEmpty());
      break;
    };
  }
  emit fieldChanged(AField,AValue);
}

void InfoWidget::setStreamJid(const Jid &AStreamJid)
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  initialize();
  autoSetFields();
  emit streamJidChanged(befour);
}

void InfoWidget::setContactJid(const Jid &AContactJid)
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  autoSetFields();
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
        disconnect(FRoster->instance(),SIGNAL(itemPush(IRosterItem *)),
          this, SLOT(onRosterItemPush(IRosterItem *)));
      }

      FRoster = rosterPlugin->getRoster(FStreamJid);
      if (FRoster)
      {
        connect(FRoster->instance(),SIGNAL(itemPush(IRosterItem *)),SLOT(onRosterItemPush(IRosterItem *)));
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
        disconnect(FPresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),
          this, SLOT(onPresenceItem(IPresenceItem *)));
      }

      FPresence = presencePlugin->getPresence(FStreamJid);
      if (FPresence)
      {
        connect(FPresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *)));
      }
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
  case IPresence::DoNotDistrib:
    return tr("Do not Distrib");
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

void InfoWidget::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == "name")
    setField(AccountName,AValue);
}

void InfoWidget::onRosterItemPush(IRosterItem *ARosterItem)
{
  if (ARosterItem->jid() == FContactJid.bare())
    setField(ContactName,ARosterItem->name());
}

void InfoWidget::onPresenceItem(IPresenceItem *APresenceItem)
{
  if (APresenceItem->jid() == FContactJid)
  {
    setField(ContactShow,showName(APresenceItem->show()));
    setField(ContactStatus,APresenceItem->status());
  }
}

