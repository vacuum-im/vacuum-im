#include "combinecontactsdialog.h"

#include <utils/logger.h>

CombineContactsDialog::CombineContactsDialog(IMetaContacts *AMetaContacts, const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FMetaContacts = AMetaContacts;

	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AContacts.count()==AMetas.count())
	{
		for (int i=0; i<AStreams.count(); i++)
		{
			Jid streamJid = AStreams.at(i);
			QUuid &metaId = FStreamMeta[streamJid];
			QList<Jid> &contacts = FStreamContacts[streamJid];

			IMetaContact meta = FMetaContacts->findMetaContact(streamJid,QUuid(AMetas.at(i)));
			if (!meta.id.isNull())
			{
				foreach(const Jid &itemJid, meta.items)
				{
					if (!contacts.contains(itemJid))
					{
						contacts.append(itemJid);
						ui.lwtContacts->addItem(contacts.last().uBare());
					}
				}
				metaId = metaId.isNull() ? meta.id : metaId;
			}
			else if (!contacts.contains(AContacts.at(i)))
			{
				contacts.append(AContacts.at(i));
				ui.lwtContacts->addItem(contacts.last().uBare());
			}
		}

		connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogButtonsBoxAccepted()));
		connect(ui.btbButtons,SIGNAL(rejected()),SLOT(onDialogButtonsBoxRejected()));
	}
	else
	{
		REPORT_ERROR("Failed to show combine contacts dialog: Invalid Params");
		deleteLater();
	}
}

void CombineContactsDialog::onDialogButtonsBoxAccepted()
{
	for (QMap<Jid, QList<Jid> >::const_iterator it=FStreamContacts.constBegin(); it!=FStreamContacts.constEnd(); ++it)
	{
		QUuid metaId = FStreamMeta.value(it.key());
		if (!metaId.isNull())
		{
			FMetaContacts->setMetaContactItems(it.key(),metaId,it.value());
			FMetaContacts->setMetaContactName(it.key(),metaId,ui.lneName->text());
		}
		else
		{
			FMetaContacts->createMetaContact(it.key(),it.value(),ui.lneName->text());
		}
	}
	close();
}

void CombineContactsDialog::onDialogButtonsBoxRejected()
{
	close();
}
