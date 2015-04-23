#include "vcarddialog.h"

#include <QBuffer>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/vcardvaluenames.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

VCardDialog::VCardDialog(IVCardManager *AVCardPlugin, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Profile - %1").arg(AContactJid.uFull()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_VCARD,0,0,"windowIcon");

	FContactJid = AContactJid;
	FStreamJid = AStreamJid;
	FVCardManager = AVCardPlugin;

	FSaveClicked = false;

	ui.cmbGender->addItem(tr("<Unset>"),QString());
	ui.cmbGender->addItem(tr("Male"),QString(VCARD_GENDER_MALE));
	ui.cmbGender->addItem(tr("Female"),QString(VCARD_GENDER_FEMALE));

	if (FStreamJid && FContactJid)
		ui.btbButtons->setStandardButtons(QDialogButtonBox::Save|QDialogButtonBox::Close);
	else
		ui.btbButtons->setStandardButtons(QDialogButtonBox::Close);
	ui.btbButtons->addButton(tr("Reload"),QDialogButtonBox::ResetRole);
	connect(ui.btbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	FVCard = FVCardManager->getVCard(FContactJid);
	connect(FVCard->instance(),SIGNAL(vcardUpdated()),SLOT(onVCardUpdated()));
	connect(FVCard->instance(),SIGNAL(vcardPublished()),SLOT(onVCardPublished()));
	connect(FVCard->instance(),SIGNAL(vcardError(const XmppError &)),SLOT(onVCardError(const XmppError &)));

	connect(ui.tlbPhotoSave,SIGNAL(clicked()),SLOT(onPhotoSaveClicked()));
	connect(ui.tlbPhotoLoad,SIGNAL(clicked()),SLOT(onPhotoLoadClicked()));
	connect(ui.tlbPhotoClear,SIGNAL(clicked()),SLOT(onPhotoClearClicked()));
	connect(ui.tlbLogoSave,SIGNAL(clicked()),SLOT(onLogoSaveClicked()));
	connect(ui.tlbLogoLoad,SIGNAL(clicked()),SLOT(onLogoLoadClicked()));
	connect(ui.tlbLogoClear,SIGNAL(clicked()),SLOT(onLogoClearClicked()));
	connect(ui.tlbEmailAdd,SIGNAL(clicked()),SLOT(onEmailAddClicked()));
	connect(ui.tlbEmailDelete,SIGNAL(clicked()),SLOT(onEmailDeleteClicked()));
	connect(ui.ltwEmails,SIGNAL(itemDoubleClicked(QListWidgetItem *)),SLOT(onEmailItemDoubleClicked(QListWidgetItem *)));
	connect(ui.tlbPhoneAdd,SIGNAL(clicked()),SLOT(onPhoneAddClicked()));
	connect(ui.tlbPhoneDelete,SIGNAL(clicked()),SLOT(onPhoneDeleteClicked()));
	connect(ui.ltwPhones,SIGNAL(itemDoubleClicked(QListWidgetItem *)),SLOT(onPhoneItemDoubleClicked(QListWidgetItem *)));
	
	if (FVCard->isEmpty())
	{
		if (FVCard->update(FStreamJid))
		{
			ui.twtVCard->setEnabled(false);
			ui.btbButtons->setEnabled(false);
		}
		else
		{
			onVCardError(tr("Service unavailable"));
		}
	}

	ui.twtVCard->setCurrentIndex(0);
	updateDialog();
}

VCardDialog::~VCardDialog()
{
	FVCard->unlock();
}

Jid VCardDialog::streamJid() const
{
	return FStreamJid;
}

Jid VCardDialog::contactJid() const
{
	return FContactJid;
}

void VCardDialog::updateDialog()
{
	bool readOnly = !(FContactJid && FStreamJid);

	ui.lneFullName->setText(FVCard->value(VVN_FULL_NAME));
	ui.lneFullName->setReadOnly(readOnly);
	ui.lneFirstName->setText(FVCard->value(VVN_GIVEN_NAME));
	ui.lneFirstName->setReadOnly(readOnly);
	ui.lneMiddleName->setText(FVCard->value(VVN_MIDDLE_NAME));
	ui.lneMiddleName->setReadOnly(readOnly);
	ui.lneLastName->setText(FVCard->value(VVN_FAMILY_NAME));
	ui.lneLastName->setReadOnly(readOnly);
	ui.lneNickName->setText(FVCard->value(VVN_NICKNAME));
	ui.lneNickName->setReadOnly(readOnly);
	ui.lneJabberId->setText(FContactJid.uFull());
	ui.lneJabberId->setReadOnly(true);

	QDate birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::ISODate);
	if (!birthday.isValid())
		birthday = QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::TextDate);
	if (!birthday.isValid() || birthday<ui.dedBirthday->minimumDate())
		birthday = ui.dedBirthday->minimumDate();
	ui.dedBirthday->setDate(birthday);
	ui.dedBirthday->setReadOnly(readOnly);
	ui.dedBirthday->setEnabled(!readOnly || birthday.isValid());
	ui.dedBirthday->setCalendarPopup(!readOnly);
	int genderIndex = ui.cmbGender->findData(FVCard->value(VVN_GENDER),Qt::UserRole,Qt::MatchExactly);
	ui.cmbGender->setCurrentIndex(genderIndex>=0 ? genderIndex : 0);
	ui.cmbGender->setEnabled(!readOnly);
	ui.lneMarital->setText(FVCard->value(VVN_MARITAL_STATUS));
	ui.lneMarital->setReadOnly(readOnly);
	ui.lneTitle->setText(FVCard->value(VVN_TITLE));
	ui.lneTitle->setReadOnly(readOnly);
	ui.lneDepartment->setText(FVCard->value(VVN_ORG_UNIT));
	ui.lneDepartment->setReadOnly(readOnly);
	ui.lneCompany->setText(FVCard->value(VVN_ORG_NAME));
	ui.lneCompany->setReadOnly(readOnly);
	ui.lneRole->setText(FVCard->value(VVN_ROLE));
	ui.lneRole->setReadOnly(readOnly);
	ui.lneHomePage->setText(FVCard->value(VVN_URL));
	ui.lneHomePage->setReadOnly(readOnly);

	static const QStringList tagHome = QStringList() << "HOME";
	static const QStringList tagWork = QStringList() << "WORK";
	static const QStringList tagsAdres = tagHome+tagWork;
	ui.lneHomeStreet->setText(FVCard->value(VVN_ADR_STREET,tagHome,tagsAdres));
	ui.lneHomeStreet->setReadOnly(readOnly);
	ui.lneHomeCity->setText(FVCard->value(VVN_ADR_CITY,tagHome,tagsAdres));
	ui.lneHomeCity->setReadOnly(readOnly);
	ui.lneHomeState->setText(FVCard->value(VVN_ADR_REGION,tagHome,tagsAdres));
	ui.lneHomeState->setReadOnly(readOnly);
	ui.lneHomeZip->setText(FVCard->value(VVN_ADR_PCODE,tagHome,tagsAdres));
	ui.lneHomeZip->setReadOnly(readOnly);
	ui.lneHomeCountry->setText(FVCard->value(VVN_ADR_COUNTRY,tagHome,tagsAdres));
	ui.lneHomeCountry->setReadOnly(readOnly);
	ui.lneWorkStreet->setText(FVCard->value(VVN_ADR_STREET,tagWork,tagsAdres));
	ui.lneWorkStreet->setReadOnly(readOnly);
	ui.lneWorkCity->setText(FVCard->value(VVN_ADR_CITY,tagWork,tagsAdres));
	ui.lneWorkCity->setReadOnly(readOnly);
	ui.lneWorkState->setText(FVCard->value(VVN_ADR_REGION,tagWork,tagsAdres));
	ui.lneWorkState->setReadOnly(readOnly);
	ui.lneWorkZip->setText(FVCard->value(VVN_ADR_PCODE,tagWork,tagsAdres));
	ui.lneWorkZip->setReadOnly(readOnly);
	ui.lneWorkCountry->setText(FVCard->value(VVN_ADR_COUNTRY,tagWork,tagsAdres));
	ui.lneWorkCountry->setReadOnly(readOnly);

	ui.ltwEmails->clear();
	static const QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
	QHash<QString,QStringList> emails = FVCard->values(VVN_EMAIL,emailTagList);
	foreach(const QString &email, emails.keys())
	{
		QListWidgetItem *listItem = new QListWidgetItem(email,ui.ltwEmails);
		listItem->setData(Qt::UserRole,emails.value(email));
		ui.ltwEmails->addItem(listItem);
	}
	ui.tlbEmailAdd->setVisible(!readOnly);
	ui.tlbEmailDelete->setVisible(!readOnly);

	ui.ltwPhones->clear();
	static const QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	QHash<QString,QStringList> phones = FVCard->values(VVN_TELEPHONE,phoneTagList);
	foreach(const QString &phone, phones.keys())
	{
		QListWidgetItem *listItem = new QListWidgetItem(phone,ui.ltwPhones);
		listItem->setData(Qt::UserRole,phones.value(phone));
		ui.ltwPhones->addItem(listItem);
	}
	ui.tlbPhoneAdd->setVisible(!readOnly);
	ui.tlbPhoneDelete->setVisible(!readOnly);

	setLogo(QByteArray::fromBase64(FVCard->value(VVN_LOGO_VALUE).toLatin1()));
	ui.tlbLogoClear->setVisible(!readOnly);
	ui.tlbLogoLoad->setVisible(!readOnly);

	setPhoto(QByteArray::fromBase64(FVCard->value(VVN_PHOTO_VALUE).toLatin1()));
	ui.tlbPhotoClear->setVisible(!readOnly);
	ui.tlbPhotoLoad->setVisible(!readOnly);

	ui.tedComments->setPlainText(FVCard->value(VVN_DESCRIPTION));
	ui.tedComments->setReadOnly(readOnly);
}

void VCardDialog::updateVCard()
{
	FVCard->clear();

	FVCard->setValueForTags(VVN_FULL_NAME,ui.lneFullName->text());
	FVCard->setValueForTags(VVN_GIVEN_NAME,ui.lneFirstName->text());
	FVCard->setValueForTags(VVN_MIDDLE_NAME,ui.lneMiddleName->text());
	FVCard->setValueForTags(VVN_FAMILY_NAME,ui.lneLastName->text());
	FVCard->setValueForTags(VVN_NICKNAME,ui.lneNickName->text());

	if (ui.dedBirthday->date() > ui.dedBirthday->minimumDate())
		FVCard->setValueForTags(VVN_BIRTHDAY,ui.dedBirthday->date().toString(Qt::ISODate));
	else
		FVCard->setValueForTags(VVN_BIRTHDAY,QString::null);
	FVCard->setValueForTags(VVN_GENDER,ui.cmbGender->itemData(ui.cmbGender->currentIndex()).toString());
	FVCard->setValueForTags(VVN_MARITAL_STATUS,ui.lneMarital->text());
	FVCard->setValueForTags(VVN_TITLE,ui.lneTitle->text());
	FVCard->setValueForTags(VVN_ORG_UNIT,ui.lneDepartment->text());
	FVCard->setValueForTags(VVN_ORG_NAME,ui.lneCompany->text());
	FVCard->setValueForTags(VVN_ROLE,ui.lneRole->text());
	FVCard->setValueForTags(VVN_URL,ui.lneHomePage->text());

	static const QStringList adresTags = QStringList() << "WORK" << "HOME";
	static const QStringList tagHome = QStringList() << "HOME";
	static const QStringList tagWork = QStringList() << "WORK";
	FVCard->setValueForTags(VVN_ADR_STREET,ui.lneHomeStreet->text(),tagHome,adresTags);
	FVCard->setValueForTags(VVN_ADR_CITY,ui.lneHomeCity->text(),tagHome,adresTags);
	FVCard->setValueForTags(VVN_ADR_REGION,ui.lneHomeState->text(),tagHome,adresTags);
	FVCard->setValueForTags(VVN_ADR_PCODE,ui.lneHomeZip->text(),tagHome,adresTags);
	FVCard->setValueForTags(VVN_ADR_COUNTRY,ui.lneHomeCountry->text(),tagHome,adresTags);
	FVCard->setValueForTags(VVN_ADR_STREET,ui.lneWorkStreet->text(),tagWork,adresTags);
	FVCard->setValueForTags(VVN_ADR_CITY,ui.lneWorkCity->text(),tagWork,adresTags);
	FVCard->setValueForTags(VVN_ADR_REGION,ui.lneWorkState->text(),tagWork,adresTags);
	FVCard->setValueForTags(VVN_ADR_PCODE,ui.lneWorkZip->text(),tagWork,adresTags);
	FVCard->setValueForTags(VVN_ADR_COUNTRY,ui.lneWorkCountry->text(),tagWork,adresTags);

	static const QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
	for (int i = 0; i<ui.ltwEmails->count(); i++)
	{
		QListWidgetItem *listItem = ui.ltwEmails->item(i);
		FVCard->setTagsForValue(VVN_EMAIL,listItem->text(),listItem->data(Qt::UserRole).toStringList(),emailTagList);
	}

	static const QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	for (int i = 0; i<ui.ltwPhones->count(); i++)
	{
		QListWidgetItem *listItem = ui.ltwPhones->item(i);
		FVCard->setTagsForValue(VVN_TELEPHONE,listItem->text(),listItem->data(Qt::UserRole).toStringList(),phoneTagList);
	}

	if (!FLogo.isEmpty())
	{
		FVCard->setValueForTags(VVN_LOGO_VALUE,FLogo.toBase64());
		FVCard->setValueForTags(VVN_LOGO_TYPE,QString("image/%1").arg(getImageFormat(FLogo).toLower()));
	}
	if (!FPhoto.isEmpty())
	{
		FVCard->setValueForTags(VVN_PHOTO_VALUE,FPhoto.toBase64());
		FVCard->setValueForTags(VVN_PHOTO_TYPE,QString("image/%1").arg(getImageFormat(FPhoto).toLower()));
	}

	FVCard->setValueForTags(VVN_DESCRIPTION,ui.tedComments->toPlainText());
}

void VCardDialog::setPhoto(const QByteArray &APhoto)
{
	QPixmap pixmap;
	if (APhoto.isEmpty() || pixmap.loadFromData(APhoto))
	{
		FPhoto = APhoto;
		ui.pmfPhoto->setImageData(FPhoto);
		ui.tlbPhotoSave->setVisible(!pixmap.isNull());
		ui.lblPhotoSize->setVisible(!pixmap.isNull());
		ui.lblPhotoSize->setText(tr("Size: %1 Kb").arg(FPhoto.size() / 1024));
	}
}

void VCardDialog::setLogo(const QByteArray &ALogo)
{
	QPixmap pixmap;
	if (ALogo.isEmpty() || pixmap.loadFromData(ALogo))
	{
		FLogo = ALogo;
		ui.pmfLogo->setImageData(FLogo);
		ui.tlbLogoSave->setVisible(!pixmap.isNull());
		ui.lblLogoSize->setVisible(!pixmap.isNull());
		ui.lblLogoSize->setText(tr("Size: %1 Kb").arg(FLogo.size() / 1024));
	}
}

QString VCardDialog::getImageFormat(const QByteArray &AData) const
{
	QBuffer buffer;
	buffer.setData(AData);
	buffer.open(QBuffer::ReadOnly);
	QByteArray format = QImageReader::imageFormat(&buffer);
	return QString::fromLocal8Bit(format.constData(),format.size());
}

QByteArray VCardDialog::loadFromFile(const QString &AFileName) const
{
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
		return file.readAll();
	return QByteArray();
}

bool VCardDialog::saveToFile(const QString &AFileName, const QByteArray &AData) const
{
	QFile file(AFileName);
	if (file.open(QFile::WriteOnly|QFile::Truncate))
	{
		file.write(AData);
		file.close();
		return true;
	}
	return false;
}

void VCardDialog::onVCardUpdated()
{
	ui.btbButtons->setEnabled(true);
	ui.twtVCard->setEnabled(true);
	updateDialog();
}

void VCardDialog::onVCardPublished()
{
	if (!FSaveClicked)
	{
		ui.btbButtons->setEnabled(true);
		ui.twtVCard->setEnabled(true);
	}
	else
	{
		accept();
	}
}

void VCardDialog::onVCardError(const XmppError &AError)
{
	QMessageBox::critical(this,tr("Error"),
		streamJid().pBare() != contactJid().pBare() ? 
		tr("Failed to load profile: %1").arg(AError.errorMessage().toHtmlEscaped()) :
		tr("Failed to publish your profile: %1").arg(AError.errorMessage().toHtmlEscaped()));

	if (!FSaveClicked)
		deleteLater();

	FSaveClicked = false;
	ui.twtVCard->setEnabled(true);
	ui.btbButtons->setEnabled(true);
}

void VCardDialog::onUpdateDialogTimeout()
{
	updateDialog();
}

void VCardDialog::onPhotoSaveClicked()
{
	if (!FPhoto.isEmpty())
	{
		QString format = getImageFormat(FPhoto).toLower();
		QString filename = QString("%1_photo.%2").arg(FContactJid.uNode()).arg(format);
		filename = QFileDialog::getSaveFileName(this,tr("Save image"),filename,tr("Image Files (*.%1)").arg(format));
		if (!filename.isEmpty())
			saveToFile(filename,FPhoto);
	}
}

void VCardDialog::onPhotoLoadClicked()
{
	QString filename = QFileDialog::getOpenFileName(this,tr("Open image"),QString::null,tr("Image Files (*.png *.jpg *.bmp *.gif)"));
	if (!filename.isEmpty())
		setPhoto(loadFromFile(filename));
}

void VCardDialog::onPhotoClearClicked()
{
	setPhoto(QByteArray());
}

void VCardDialog::onLogoSaveClicked()
{
	if (!FLogo.isNull())
	{
		QString format = getImageFormat(FPhoto).toLower();
		QString filename = QString("%1_logo.%2").arg(FContactJid.uNode()).arg(format);
		filename = QFileDialog::getSaveFileName(this,tr("Save image"),filename,tr("Image Files (*.%1)").arg(format));
		if (!filename.isEmpty())
			saveToFile(filename,FLogo);
	}
}

void VCardDialog::onLogoLoadClicked()
{
	QString filename = QFileDialog::getOpenFileName(this,tr("Open image"),QString::null,tr("Image Files (*.png *.jpg *.bmp *.gif)"));
	if (!filename.isEmpty())
		setLogo(loadFromFile(filename));
}

void VCardDialog::onLogoClearClicked()
{
	setLogo(QByteArray());
}

void VCardDialog::onEmailAddClicked()
{
	static QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
	EditItemDialog dialog(QString::null,QStringList(),emailTagList,this);
	dialog.setLabelText(tr("EMail:"));
	if (dialog.exec()==QDialog::Accepted && !dialog.value().isEmpty() && ui.ltwEmails->findItems(dialog.value(),Qt::MatchFixedString).isEmpty())
	{
		QListWidgetItem *item = new QListWidgetItem(dialog.value(),ui.ltwEmails);
		item->setData(Qt::UserRole,dialog.tags());
		ui.ltwEmails->addItem(item);
	}
}

void VCardDialog::onEmailDeleteClicked()
{
	QListWidgetItem *item = ui.ltwEmails->takeItem(ui.ltwEmails->currentRow());
	delete item;
}

void VCardDialog::onEmailItemDoubleClicked(QListWidgetItem *AItem)
{
	if (FStreamJid && FContactJid)
	{
		static QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
		EditItemDialog dialog(AItem->text(),AItem->data(Qt::UserRole).toStringList(),emailTagList,this);
		dialog.setLabelText(tr("EMail:"));
		if (dialog.exec() == QDialog::Accepted)
		{
			AItem->setText(dialog.value());
			AItem->setData(Qt::UserRole,dialog.tags());
		}
	}
}

void VCardDialog::onPhoneAddClicked()
{
	static QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
	EditItemDialog dialog(QString::null,QStringList(),phoneTagList,this);
	dialog.setLabelText(tr("Phone:"));
	if (dialog.exec()==QDialog::Accepted && !dialog.value().isEmpty() && ui.ltwPhones->findItems(dialog.value(),Qt::MatchFixedString).isEmpty())
	{
		QListWidgetItem *item = new QListWidgetItem(dialog.value(),ui.ltwPhones);
		item->setData(Qt::UserRole,dialog.tags());
		ui.ltwPhones->addItem(item);
	}
}

void VCardDialog::onPhoneDeleteClicked()
{
	QListWidgetItem *item = ui.ltwPhones->takeItem(ui.ltwPhones->currentRow());
	delete item;
}

void VCardDialog::onPhoneItemDoubleClicked(QListWidgetItem *AItem)
{
	if (FStreamJid && FContactJid)
	{
		static QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
		EditItemDialog dialog(AItem->text(),AItem->data(Qt::UserRole).toStringList(),phoneTagList,this);
		dialog.setLabelText(tr("Phone:"));
		if (dialog.exec() == QDialog::Accepted)
		{
			AItem->setText(dialog.value());
			AItem->setData(Qt::UserRole,dialog.tags());
		}
	}
}

void VCardDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	if (ui.btbButtons->standardButton(AButton) == QDialogButtonBox::Close)
	{
		close();
	}
	else if (ui.btbButtons->standardButton(AButton) == QDialogButtonBox::Save)
	{
		updateVCard();
		if (FVCard->publish(FStreamJid))
		{
			ui.btbButtons->setEnabled(false);
			ui.twtVCard->setEnabled(false);
			FSaveClicked = true;
		}
		else
		{
			QMessageBox::warning(this,tr("Error"),tr("Failed to publish your profile."));
		}
	}
	else if (ui.btbButtons->buttonRole(AButton) == QDialogButtonBox::ResetRole)
	{
		if (FVCard->update(FStreamJid))
		{
			ui.btbButtons->setEnabled(false);
			ui.twtVCard->setEnabled(false);
		}
		else
		{
			QMessageBox::warning(this,tr("Error"),tr("Failed to load profile."));
		}
	}
}
