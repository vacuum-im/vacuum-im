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
	CombineContactsDialog(IMetaContacts *AMetaContacts, const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent = NULL);
protected slots:
	void onDialogButtonsBoxAccepted();
	void onDialogButtonsBoxRejected();
private:
	Ui::CombineContactsDialog ui;
private:
	IMetaContacts *FMetaContacts;
private:
	QMap<Jid, QUuid> FStreamMeta;
	QMap<Jid, QList<Jid> > FStreamContacts;
};

#endif // COMBINECONTACTSDIALOG_H
