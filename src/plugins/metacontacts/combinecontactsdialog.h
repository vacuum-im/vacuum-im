#ifndef COMBINECONTACTSDIALOG_H
#define COMBINECONTACTSDIALOG_H

#include <QDialog>
#include <interfaces/irostermanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/imetacontacts.h>
#include "ui_combinecontactsdialog.h"

class CombineContactsDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	CombineContactsDialog(IMetaContacts *AMetaContacts, const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent = NULL);
protected slots:
	void onDialogButtonsBoxAccepted();
	void onDialogButtonsBoxRejected();
	void onMetaContactsClosed(const Jid &AStreamJid);
private:
	Ui::CombineContactsDialog ui;
private:
	IAvatars *FAvatars;
	IRosterManager *FRosterManager;
	IMetaContacts *FMetaContacts;
private:
	QUuid FMetaId;
	QMultiMap<Jid, Jid> FMetaItems;
};

#endif // COMBINECONTACTSDIALOG_H
