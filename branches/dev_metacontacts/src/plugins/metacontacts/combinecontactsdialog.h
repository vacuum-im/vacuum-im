#ifndef COMBINECONTACTSDIALOG_H
#define COMBINECONTACTSDIALOG_H

#include <QDialog>
#include <interfaces/imetacontacts.h>
#include "ui_combinecontactsdialog.h"

class CombineContactsDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	CombineContactsDialog(IMetaContacts *AMetaContacts, const Jid &AStreamJid, const QStringList &AContactJids, const QStringList &AMetaIds, QWidget *AParent = NULL);
	~CombineContactsDialog();
protected slots:
	void onDialogButtonsBoxAccepted();
	void onDialogButtonsBoxRejected();
private:
	Ui::CombineContactsDialog ui;
private:
	IMetaContacts *FMetaContacts;
private:
	QUuid FMetaId;
	Jid FStreamJid;
	QList<Jid> FContacts;
};

#endif // COMBINECONTACTSDIALOG_H
