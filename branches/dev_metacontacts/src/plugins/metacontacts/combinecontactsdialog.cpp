#include "combinecontactsdialog.h"

CombineContactsDialog::CombineContactsDialog(IMetaContacts *AMetaContacts, const Jid &AStreamJid, const QStringList &AContactJids, const QStringList &AMetaIds, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	FMetaContacts = AMetaContacts;
}

CombineContactsDialog::~CombineContactsDialog()
{

}
