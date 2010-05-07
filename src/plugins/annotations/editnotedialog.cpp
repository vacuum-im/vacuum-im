#include "editnotedialog.h"

EditNoteDialog::EditNoteDialog(IAnnotations *AAnnotations, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Annotation - %1").arg(AContactJid.bare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_ANNOTATIONS,0,0,"windowIcon");

	FAnnotations = AAnnotations;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;

	ui.lblCreatedValue->setText(AAnnotations->annotationCreateDate(AStreamJid,AContactJid).toString());
	ui.lblModifiedValue->setText(AAnnotations->annotationModifyDate(AStreamJid,AContactJid).toString());
	ui.pteNote->setPlainText(AAnnotations->annotation(AStreamJid,AContactJid));

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));
}

EditNoteDialog::~EditNoteDialog()
{
	emit dialogDestroyed();
}

void EditNoteDialog::onDialogAccepted()
{
	QString note = ui.pteNote->toPlainText();
	if (note != FAnnotations->annotation(FStreamJid,FContactJid))
	{
		FAnnotations->setAnnotation(FStreamJid,FContactJid,ui.pteNote->toPlainText());
		FAnnotations->saveAnnotations(FStreamJid);
	}
	accept();
}
