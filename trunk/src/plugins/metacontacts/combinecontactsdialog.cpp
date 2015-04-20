#include "combinecontactsdialog.h"

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/logger.h>
#include <utils/iconstorage.h>
#include <utils/imagemanager.h>
#include <utils/pluginhelper.h>

#define AVATAR_SIZE QSize(24,24)

static bool StringSizeLessThan(const QString &AValue1, const QString &AValue2)
{
	return AValue1.size() < AValue2.size();
}

CombineContactsDialog::CombineContactsDialog(IMetaContacts *AMetaContacts, const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_METACONTACTS_COMBINE,0,0,"windowIcon");

	FMetaContacts = AMetaContacts;
	connect(FMetaContacts->instance(),SIGNAL(metaContactsClosed(const Jid &)),SLOT(onMetaContactsClosed(const Jid &)));

	FAvatars = PluginHelper::pluginInstance<IAvatars>();
	FRosterManager = PluginHelper::pluginInstance<IRosterManager>();

	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AContacts.count()==AMetas.count())
	{
		QStringList metaNames;
		QStringList contactNames;
		QStringList contactNodes;

		for (int i=0; i<AStreams.count(); i++)
		{
			QUuid metaId = AMetas.at(i);
			Jid streamJid = AStreams.at(i);
			if (!metaId.isNull())
			{
				foreach(const Jid &metaStream, FMetaContacts->findMetaStreams(metaId))
				{
					IMetaContact meta = FMetaContacts->findMetaContact(metaStream,metaId);

					foreach(const Jid &itemJid, meta.items)
						if (!FMetaItems.contains(metaStream,itemJid))
							FMetaItems.insertMulti(metaStream,itemJid);

					if (!meta.name.isEmpty())
						metaNames.append(meta.name);
				}
				FMetaId = metaId;
			}
			else if (!FMetaItems.contains(streamJid,AContacts.at(i)))
			{
				FMetaItems.insertMulti(streamJid,AContacts.at(i));
			}
		}
		FMetaId = FMetaId.isNull() ? QUuid::createUuid() : FMetaId;

		ui.lwtContacts->setIconSize(AVATAR_SIZE);
		for (QMultiMap<Jid, Jid>::const_iterator it = FMetaItems.constBegin(); it!=FMetaItems.constEnd(); ++it)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(it.key()) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->findItem(it.value()) : IRosterItem();
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
		qSort(metaNames.begin(), metaNames.end(), StringSizeLessThan);
		if (metaNames.isEmpty())
		{
			qSort(contactNames.begin(), contactNames.end(), StringSizeLessThan);
			if (contactNames.isEmpty())
			{
				qSort(contactNodes.begin(), contactNodes.end(), StringSizeLessThan);
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

void CombineContactsDialog::onDialogButtonsBoxAccepted()
{
	foreach (const Jid &streamJid, FMetaItems.uniqueKeys())
		FMetaContacts->createMetaContact(streamJid,FMetaId,ui.lneName->text(),FMetaItems.values(streamJid));
	close();
}

void CombineContactsDialog::onDialogButtonsBoxRejected()
{
	close();
}

void CombineContactsDialog::onMetaContactsClosed(const Jid &AStreamJid)
{
	if (FMetaItems.contains(AStreamJid))
		close();
}
