#ifndef VCARDDIALOG_H
#define VCARDDIALOG_H

#include <QDialog>
#include <QEvent>
#include <QResizeEvent>
#include <definations/vcardvaluenames.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ivcard.h>
#include <utils/iconstorage.h>
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
	void reloadVCard();
	void publishVCard();
	void updateDialog();
	void updateVCard();
	void setPhoto(const QPixmap &APhoto);
	void setLogo(const QPixmap &ALogo);
	void updatePhotoLabel(const QSize &ASize);
	void updateLogoLabel(const QSize &ASize);
protected:
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onVCardUpdated();
	void onVCardPublished();
	void onVCardError(const QString &AError);
	void onUpdateClicked();
	void onPublishClicked();
	void onClearClicked();
	void onCloseClicked();
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
private:
	Ui::VCardDialogClass ui;
private:
	QPixmap FLogo;
	QPixmap FPhoto;
	Jid FContactJid;
	Jid FStreamJid;
	IVCard *FVCard;
	IVCardPlugin *FVCardPlugin;
};

#endif // VCARDDIALOG_H
