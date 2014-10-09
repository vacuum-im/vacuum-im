#ifndef COMBINECONTACTSDIALOG_H
#define COMBINECONTACTSDIALOG_H

#include <QDialog>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/ipluginmanager.h>
#include "ui_combinecontactsdialog.h"

class CombineContactsDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	CombineContactsDialog(IPluginManager *APluginManager, IMetaContacts *AMetaContacts,
		const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas, QWidget *AParent = NULL);
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onDialogButtonsBoxAccepted();
	void onDialogButtonsBoxRejected();
	void onMetaContactsClosed(const Jid &AStreamJid);
private:
	Ui::CombineContactsDialog ui;
private:
	IAvatars *FAvatars;
	IRosterPlugin *FRosterPlugin;
	IMetaContacts *FMetaContacts;
private:
	QUuid FMetaLinkId;
	QMultiMap<Jid, QUuid> FStreamMetas;
	QMultiMap<Jid, Jid> FStreamContacts;
};

#endif // COMBINECONTACTSDIALOG_H
