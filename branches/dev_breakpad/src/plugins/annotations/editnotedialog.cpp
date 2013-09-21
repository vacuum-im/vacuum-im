#include "editnotedialog.h"

EditNoteDialog::EditNoteDialog(IAnnotations *AAnnotations, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Annotation - %1").arg(AContactJid.uBare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_ANNOTATIONS,0,0,"windowIcon");

/* simple breakpad test, uses edit annotation dialog, qmake DEFINES+=WITH_BREAKPAD_TEST */
#if defined (WITH_BREAKPAD_TEST)
	FAnnotations = AAnnotations + 10;
#else
	FAnnotations = AAnnotations;
#endif
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
		FAnnotations->setAnnotation(FStreamJid,FContactJid,ui.pteNote->toPlainText());
	accept();
}
