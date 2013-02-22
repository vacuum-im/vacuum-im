#include "infowidget.h"

#include <QMovie>
#include <QImageReader>

InfoWidget::InfoWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FAccount = NULL;
	FRoster = NULL;
	FPresence = NULL;
	FAvatars = NULL;
	FStatusChanger = NULL;

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;

	FAutoFields = 0xFFFFFFFF;
	FVisibleFields = AccountName|ContactName|ContactStatus|ContactAvatar;

	initialize();
	connect(FWindow->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onAddressChanged(const Jid &, const Jid &)));
}

InfoWidget::~InfoWidget()
{

}

IMessageWindow *InfoWidget::messageWindow() const
{
	return FWindow;
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
	switch (AField)
	{
	case AccountName:
	{
		setField(AField, FAccount!=NULL ? FAccount->name() : FWindow->streamJid().uFull());
		break;
	}
	case ContactName:
	{
		QString name;
		Jid contactJid = FWindow->contactJid();
		if (FWindow->streamJid().pBare() != contactJid.pBare())
		{
			IRosterItem ritem = FRoster ? FRoster->rosterItem(contactJid) : IRosterItem();
			name = ritem.isValid && !ritem.name.isEmpty() ? ritem.name : (!contactJid.node().isEmpty() ? contactJid.uNode() : contactJid.domain());
		}
		else
		{
			name = contactJid.resource();
		}
		setField(AField,name);
		break;
	}
	case ContactShow:
	{
		setField(AField,FPresence!=NULL ? FPresence->findItem(FWindow->contactJid()).show : IPresence::Offline);
		break;
	}
	case ContactStatus:
	{
		setField(AField,FPresence!=NULL ? FPresence->findItem(FWindow->contactJid()).status : QString::null);
		break;
	}
	case ContactAvatar:
	{
		setField(AField, FAvatars!=NULL ? FAvatars->avatarFileName(FAvatars->avatarHash(FWindow->contactJid())) : QString::null);
		break;
	}
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

bool InfoWidget::isFiledAutoUpdated(IMessageInfoWidget::InfoField AField) const
{
	return (FAutoFields & AField) > 0;
}

void InfoWidget::setFieldAutoUpdated(IMessageInfoWidget::InfoField AField, bool AAuto)
{
	if (isFiledAutoUpdated(AField) != AAuto)
	{
		if (AAuto)
		{
			FAutoFields |= AField;
			autoUpdateField(AField);
		}
		else
		{
			FAutoFields &= ~AField;
		}
	}
}

int InfoWidget::visibleFields() const
{
	return FVisibleFields;
}

bool InfoWidget::isFieldVisible(IMessageInfoWidget::InfoField AField) const
{
	return (FVisibleFields & AField) >0;
}

void InfoWidget::setFieldVisible(IMessageInfoWidget::InfoField AField, bool AVisible)
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
				disconnect(FAccount->instance(),SIGNAL(optionsChanged(const OptionsNode &)), this, 
					SLOT(onAccountChanged(const OptionsNode &)));
			}

			FAccount = accountManager->accountByStream(FWindow->streamJid());
			if (FAccount)
			{
				connect(FAccount->instance(),SIGNAL(optionsChanged(const OptionsNode &)),
					SLOT(onAccountChanged(const OptionsNode &)));
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
				disconnect(FRoster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)), this, 
					SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
			}

			FRoster = rosterPlugin->findRoster(FWindow->streamJid());
			if (FRoster)
			{
				connect(FRoster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)), 
					SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
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
				disconnect(FPresence->instance(),SIGNAL(itemReceived(const IPresenceItem &, const IPresenceItem &)), this, 
					SLOT(onPresenceItemReceived(const IPresenceItem &, const IPresenceItem &)));
			}

			FPresence = presencePlugin->findPresence(FWindow->streamJid());
			if (FPresence)
			{
				connect(FPresence->instance(),SIGNAL(itemReceived(const IPresenceItem &, const IPresenceItem &)), 
					SLOT(onPresenceItemReceived(const IPresenceItem &, const IPresenceItem &)));
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

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}
}

void InfoWidget::updateFieldLabel(IMessageInfoWidget::InfoField AField)
{
	switch (AField)
	{
	case AccountName:
	{
		QString name = field(AField).toString();
		ui.lblAccount->setText(name);
		ui.lblAccount->setVisible(isFieldVisible(AField) && !name.isEmpty());
		break;
	}
	case ContactName:
	{
		QString name = field(AField).toString();

		IRosterItem ritem = FRoster ? FRoster->rosterItem(FWindow->contactJid()) : IRosterItem();
		if (isFiledAutoUpdated(AField) && ritem.name.isEmpty())
			ui.lblName->setText(Qt::escape(FWindow->contactJid().uFull()));
		else
			ui.lblName->setText(QString("<big><b>%1</b></big> - %2").arg(Qt::escape(name)).arg(Qt::escape(FWindow->contactJid().uFull())));

		ui.lblName->setVisible(isFieldVisible(AField));
		break;
	}
	case ContactShow:
	case ContactStatus:
	{
		QString status = field(AField).toString();
		ui.lblStatus->setToolTip(status);
		int maxStatusChars = Options::node(OPV_MESSAGES_INFOWIDGETMAXSTATUSCHARS).value().toInt();
		if (maxStatusChars>0 && status.length() > maxStatusChars)
			status = status.left(maxStatusChars) + "...";
		
		if (FStatusChanger)
		{
			int show = field(ContactShow).toInt();
			status = QString("[%1]").arg(FStatusChanger->nameByShow(show))+ " " +status;
		}

		ui.lblStatus->setText(status);
		ui.lblStatus->setVisible(isFieldVisible(AField) && !status.isEmpty());
		break;
	}
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
	}
	default:
		break;
	}
}

void InfoWidget::onAccountChanged(const OptionsNode &ANode)
{
	if (FAccount && isFiledAutoUpdated(AccountName) && FAccount->optionsNode().childPath(ANode)=="name")
		autoUpdateField(AccountName);
}

void InfoWidget::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (isFiledAutoUpdated(ContactName) && (AItem.itemJid && FWindow->contactJid()) && AItem.name!=ABefore.name)
		autoUpdateField(ContactName);
}

void InfoWidget::onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.itemJid==FWindow->contactJid() && (AItem.show!=ABefore.show || AItem.status!=ABefore.status))
	{
		if (isFiledAutoUpdated(ContactShow))
			setField(ContactShow,AItem.show);
		if (isFiledAutoUpdated(ContactStatus))
			setField(ContactStatus,AItem.status);
	}
}

void InfoWidget::onAvatarChanged(const Jid &AContactJid)
{
	if (isFiledAutoUpdated(ContactAvatar) && (FWindow->contactJid() && AContactJid))
		autoUpdateField(ContactAvatar);
}

void InfoWidget::onAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AContactBefore);
	if (FWindow->streamJid() != AStreamBefore)
		initialize();
	autoUpdateFields();
}
