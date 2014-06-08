#include "combinecontactsdialog.h"

CombineContactsDialog::CombineContactsDialog(IMetaContacts *AMetaContacts, const Jid &AStreamJid, const QStringList &AContactJids, const QStringList &AMetaIds, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	FStreamJid = AStreamJid;
	FMetaContacts = AMetaContacts;

	foreach(const QString &contact, AContactJids)
	{
		Jid contactJid = contact;
		if (contactJid.isValid() && !FContacts.contains(contactJid.bare()))
			FContacts.append(contactJid.bare());
	}

	foreach(const QUuid &metaId, AMetaIds)
	{
		IMetaContact meta = FMetaContacts->findMetaContact(FStreamJid,metaId);
		if (!meta.id.isNull())
		{
			foreach(const Jid &contactJid, meta.items)
				if (!FContacts.contains(contactJid))
					FContacts.append(contactJid);

			if (FMetaId.isNull())
				FMetaId = meta.id;
		}
	}

	connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogButtonsBoxAccepted()));
	connect(ui.btbButtons,SIGNAL(rejected()),SLOT(onDialogButtonsBoxRejected()));
}

CombineContactsDialog::~CombineContactsDialog()
{

}

void CombineContactsDialog::onDialogButtonsBoxAccepted()
{
	FMetaContacts->createMetaContact(FStreamJid,FContacts,ui.lneName->text());
}

void CombineContactsDialog::onDialogButtonsBoxRejected()
{
	close();
}
