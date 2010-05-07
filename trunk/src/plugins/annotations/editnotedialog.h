#ifndef EDITNOTEDIALOG_H
#define EDITNOTEDIALOG_H

#include <QDialog>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <interfaces/iannotations.h>
#include <utils/iconstorage.h>
#include "ui_editnotedialog.h"

class EditNoteDialog :
			public QDialog
{
	Q_OBJECT;
public:
	EditNoteDialog(IAnnotations *AAnnotations, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent = NULL);
	~EditNoteDialog();
	const Jid &streamJid() { return FStreamJid; }
	const Jid &contactJid() { return FContactJid; }
signals:
	void dialogDestroyed();
protected slots:
	void onDialogAccepted();
private:
	Ui::EditNoteDialogClass ui;
private:
	IAnnotations *FAnnotations;
private:
	Jid FStreamJid;
	Jid FContactJid;
};

#endif // EDITNOTEDIALOG_H
