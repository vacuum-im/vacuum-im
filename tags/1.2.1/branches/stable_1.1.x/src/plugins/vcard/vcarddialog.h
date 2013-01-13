#ifndef VCARDDIALOG_H
#define VCARDDIALOG_H

#include <QDialog>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ivcard.h>
#include <utils/iconstorage.h>
#include "edititemdialog.h"
#include "ui_vcarddialog.h"

class VCardDialog :
			public QDialog
{
	Q_OBJECT;
public:
	VCardDialog(IVCardPlugin *AVCardPlugin,const Jid &AStreamJid, const Jid &AContactJid);
	~VCardDialog();
	virtual Jid streamJid() const { return FStreamJid; }
	virtual Jid contactJid() const { return FContactJid; }
protected:
	void updateDialog();
	void updateVCard();
	void setPhoto(const QPixmap &APhoto);
	void setLogo(const QPixmap &ALogo);
protected slots:
	void onVCardUpdated();
	void onVCardPublished();
	void onVCardError(const QString &AError);
	void onUpdateDialogTimeout();
	void onPhotoSaveClicked();
	void onPhotoLoadClicked();
	void onPhotoClearClicked();
	void onLogoSaveClicked();
	void onLogoLoadClicked();
	void onLogoClearClicked();
	void onEmailAddClicked();
	void onEmailDeleteClicked();
	void onEmailItemActivated(QListWidgetItem *AItem);
	void onPhoneAddClicked();
	void onPhoneDeleteClicked();
	void onPhoneItemActivated(QListWidgetItem *AItem);
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	Ui::VCardDialogClass ui;
private:
	IVCard *FVCard;
	IVCardPlugin *FVCardPlugin;
private:
	Jid FContactJid;
	Jid FStreamJid;
	QPixmap FLogo;
	QPixmap FPhoto;
	bool FSaveClicked;
};

#endif // VCARDDIALOG_H
