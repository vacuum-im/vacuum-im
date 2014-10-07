#include "combinecontactsdialog.h"

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/logger.h>
#include <utils/iconstorage.h>
#include <utils/imagemanager.h>

#define AVATAR_SIZE QSize(32,32)

static bool SizeLessThan(const QString &AValue1, const QString &AValue2)
{
	return AValue1.size() < AValue2.size();
}

CombineContactsDialog::CombineContactsDialog(IPluginManager *APluginManager, IMetaContacts *AMetaContacts,
	const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_METACONTACTS_COMBINE,0,0,"windowIcon");

	FAvatars = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = AMetaContacts;
	initialize(APluginManager);

	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AContacts.count()==AMetas.count())
	{
		QStringList metaNames;
		QStringList contactNames;
		QStringList contactNodes;

		for (int i=0; i<AStreams.count(); i++)
		{
			Jid streamJid = AStreams.at(i);

			IMetaContact meta = FMetaContacts->findMetaContact(streamJid,QUuid(AMetas.at(i)));
			if (!meta.isNull())
			{
				foreach(const Jid &itemJid, meta.items)
					if (!FStreamContacts.contains(streamJid,itemJid))
						FStreamContacts.insertMulti(streamJid,itemJid);

				if (!meta.name.isEmpty())
					metaNames.append(meta.name);
				FStreamMetas.insertMulti(streamJid,meta.id);
			}
			else if (!FStreamContacts.contains(streamJid,AContacts.at(i)))
			{
				FStreamContacts.insertMulti(streamJid,AContacts.at(i));
			}
		}

		ui.lwtContacts->setIconSize(AVATAR_SIZE);
		for (QMultiMap<Jid, Jid>::const_iterator it = FStreamContacts.constBegin(); it!=FStreamContacts.constEnd(); ++it)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(it.key()) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->rosterItem(it.value()) : IRosterItem();
			QString name = !ritem.name.isEmpty() ? ritem.name : ritem.itemJid.uBare();

			QImage avatar = FAvatars!=NULL ? FAvatars->loadAvatarImage(FAvatars->avatarHash(it.value()),AVATAR_SIZE) : QImage();
			avatar = avatar.isNull() ? (FAvatars!=NULL ? FAvatars->emptyAvatarImage(AVATAR_SIZE) : QImage()) : avatar;
			avatar = ImageManager::squared(avatar,AVATAR_SIZE.width());

			QListWidgetItem *item = new QListWidgetItem(QIcon(QPixmap::fromImage(avatar)),name,ui.lwtContacts);
			item->setFlags(Qt::ItemIsEnabled);
			ui.lwtContacts->addItem(item);

			if (!ritem.name.isEmpty())
				contactNames.append(name);
			else
				contactNodes.append(ritem.itemJid.uNode());
		}

		QString name;
		qSort(metaNames.begin(), metaNames.end(), SizeLessThan);
		if (metaNames.isEmpty())
		{
			qSort(contactNames.begin(), contactNames.end(), SizeLessThan);
			if (contactNames.isEmpty())
			{
				qSort(contactNodes.begin(), contactNodes.end(), SizeLessThan);
				name = !contactNodes.isEmpty() ? contactNodes.last(): name;
			}
			else
			{
				name = contactNames.last();
			}
		}
		else
		{
			name = metaNames.last();
		}
		ui.lneName->setText(name);
		ui.lneName->selectAll();

		connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogButtonsBoxAccepted()));
		connect(ui.btbButtons,SIGNAL(rejected()),SLOT(onDialogButtonsBoxRejected()));
	}
	else
	{
		REPORT_ERROR("Failed to show combine contacts dialog: Invalid Params");
		deleteLater();
	}

	ui.lblNotice->setText(tr("The following <b>%n contact(s)</b> will be merged into metacontact:","",ui.lwtContacts->count()));
}


void CombineContactsDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	connect(FMetaContacts->instance(),SIGNAL(metaContactsClosed(const Jid &)),SLOT(onMetaContactsClosed(const Jid &)));
}

void CombineContactsDialog::onDialogButtonsBoxAccepted()
{
	foreach (const Jid &streamJid, FStreamContacts.uniqueKeys())
	{
		QUuid metaId = FStreamMetas.value(streamJid);
		if (!metaId.isNull())
		{
			FMetaContacts->setMetaContactItems(streamJid,metaId,FStreamContacts.values(streamJid));
			FMetaContacts->setMetaContactName(streamJid,metaId,ui.lneName->text());
		}
		else
		{
			FMetaContacts->createMetaContact(streamJid,FStreamContacts.values(streamJid),ui.lneName->text());
		}
	}
	close();
}

void CombineContactsDialog::onDialogButtonsBoxRejected()
{
	close();
}

void CombineContactsDialog::onMetaContactsClosed(const Jid &AStreamJid)
{
	if (FStreamContacts.contains(AStreamJid))
		close();
}
