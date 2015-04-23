#ifndef VCARDDIALOG_H
#define VCARDDIALOG_H

#include <QDialog>
#include <interfaces/ivcardmanager.h>
#include "edititemdialog.h"
#include "ui_vcarddialog.h"

class VCardDialog :
	public QDialog
{
	Q_OBJECT;
public:
	VCardDialog(IVCardManager *AVCardPlugin,const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
	~VCardDialog();
	Jid streamJid() const;
	Jid contactJid() const;
protected:
	void updateDialog();
	void updateVCard();
	void setPhoto(const QByteArray &APhoto);
	void setLogo(const QByteArray &ALogo);
	QString getImageFormat(const QByteArray &AData) const;
	QByteArray loadFromFile(const QString &AFileName) const;
	bool saveToFile(const QString &AFileName, const QByteArray &AData) const;
protected slots:
	void onVCardUpdated();
	void onVCardPublished();
	void onVCardError(const XmppError &AError);
	void onUpdateDialogTimeout();
	void onPhotoSaveClicked();
	void onPhotoLoadClicked();
	void onPhotoClearClicked();
	void onLogoSaveClicked();
	void onLogoLoadClicked();
	void onLogoClearClicked();
	void onEmailAddClicked();
	void onEmailDeleteClicked();
	void onEmailItemDoubleClicked(QListWidgetItem *AItem);
	void onPhoneAddClicked();
	void onPhoneDeleteClicked();
	void onPhoneItemDoubleClicked(QListWidgetItem *AItem);
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	Ui::VCardDialogClass ui;
private:
	IVCard *FVCard;
	IVCardManager *FVCardManager;
private:
	Jid FContactJid;
	Jid FStreamJid;
	QByteArray FLogo;
	QByteArray FPhoto;
	bool FSaveClicked;
};

#endif // VCARDDIALOG_H
