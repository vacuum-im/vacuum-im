#include "vcarddialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include "edititemdialog.h"

#define IN_VCARD                  "psi/vCard"

VCardDialog::VCardDialog(IVCardPlugin *AVCardPlugin, const Jid &AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  ui.lblPhoto->installEventFilter(this);
  ui.lblLogo->installEventFilter(this);
  
  FContactJid = AContactJid;
  FStreamJid = AStreamJid;
  FVCardPlugin = AVCardPlugin;

  SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  setWindowIcon(iconset->iconByName(IN_VCARD));
  setWindowTitle(tr("vCard - %1").arg(FContactJid.bare()));

  ui.pbtPublish->setVisible(FContactJid && FStreamJid);
  ui.pbtClear->setVisible(FContactJid && FStreamJid);

  FVCard = FVCardPlugin->vcard(FContactJid);
  connect(FVCard->instance(),SIGNAL(vcardUpdated()),SLOT(onVCardUpdated()));
  connect(FVCard->instance(),SIGNAL(vcardPublished()),SLOT(onVCardPublished()));
  connect(FVCard->instance(),SIGNAL(vcardError(const QString &)),SLOT(onVCardError(const QString &)));
  
  updateDialog();
  if (FVCard->isEmpty())
    reloadVCard();

  connect(ui.pbtUpdate,SIGNAL(clicked()),SLOT(onUpdateClicked()));
  connect(ui.pbtPublish,SIGNAL(clicked()),SLOT(onPublishClicked()));
  connect(ui.pbtClear,SIGNAL(clicked()),SLOT(onClearClicked()));
  connect(ui.pbtClose,SIGNAL(clicked()),SLOT(onCloseClicked()));
  connect(ui.tlbPhotoSave,SIGNAL(clicked()),SLOT(onPhotoSaveClicked()));
  connect(ui.tlbPhotoLoad,SIGNAL(clicked()),SLOT(onPhotoLoadClicked()));
  connect(ui.tlbPhotoClear,SIGNAL(clicked()),SLOT(onPhotoClearClicked()));
  connect(ui.tlbLogoSave,SIGNAL(clicked()),SLOT(onLogoSaveClicked()));
  connect(ui.tlbLogoLoad,SIGNAL(clicked()),SLOT(onLogoLoadClicked()));
  connect(ui.tlbLogoClear,SIGNAL(clicked()),SLOT(onLogoClearClicked()));
  connect(ui.tlbEmailAdd,SIGNAL(clicked()),SLOT(onEmailAddClicked()));
  connect(ui.tlbEmailDelete,SIGNAL(clicked()),SLOT(onEmailDeleteClicked()));
  connect(ui.ltwEmails,SIGNAL(itemActivated(QListWidgetItem *)),SLOT(onEmailItemActivated(QListWidgetItem *)));
  connect(ui.tlbPhoneAdd,SIGNAL(clicked()),SLOT(onPhoneAddClicked()));
  connect(ui.tlbPhoneDelete,SIGNAL(clicked()),SLOT(onPhoneDeleteClicked()));
  connect(ui.ltwPhones,SIGNAL(itemActivated(QListWidgetItem *)),SLOT(onPhoneItemActivated(QListWidgetItem *)));
}

VCardDialog::~VCardDialog()
{
  FVCard->unlock();
}

void VCardDialog::reloadVCard()
{
  if (FVCard->update(FStreamJid))
  {
    ui.pbtUpdate->setEnabled(false);
    ui.pbtPublish->setEnabled(false);
    ui.pbtClear->setEnabled(false);
    ui.twtVCard->setEnabled(false);
  }
}

void VCardDialog::publishVCard()
{
  if (FVCard->publish(FStreamJid))
  {
    ui.pbtUpdate->setEnabled(false);
    ui.pbtPublish->setEnabled(false);
    ui.pbtClear->setEnabled(false);
    ui.twtVCard->setEnabled(false);
  }
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
  ui.lneJabberId->setText(FContactJid.bare());
  ui.lneJabberId->setReadOnly(true);

  ui.dedBirthday->setDate(QDate::fromString(FVCard->value(VVN_BIRTHDAY),Qt::ISODate));
  ui.dedBirthday->setReadOnly(readOnly);
  ui.dedBirthday->setCalendarPopup(!readOnly);
  ui.cmbGender->lineEdit()->setText(FVCard->value(VVN_GENDER));
  ui.cmbGender->setEnabled(!readOnly);
  ui.lneMaritial->setText(FVCard->value(VVN_MARITALSTATUS));
  ui.lneMaritial->setReadOnly(readOnly);
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

  static QStringList tagHome = QStringList() << "HOME";
  static QStringList tagWork = QStringList() << "WORK";
  static QStringList tagsAdres = tagHome+tagWork;
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
  static QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
  QHash<QString,QStringList> emails = FVCard->values(VVN_EMAIL,emailTagList);
  foreach(QString email, emails.keys())
  {
    QListWidgetItem *listItem = new QListWidgetItem(email,ui.ltwEmails);
    listItem->setData(Qt::UserRole,emails.value(email));
    ui.ltwEmails->addItem(listItem);
  }
  ui.tlbEmailAdd->setVisible(!readOnly);
  ui.tlbEmailDelete->setVisible(!readOnly);

  ui.ltwPhones->clear();
  static QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
  QHash<QString,QStringList> phones = FVCard->values(VVN_TELEPHONE,phoneTagList);
  foreach(QString phone, phones.keys())
  {
    QListWidgetItem *listItem = new QListWidgetItem(phone,ui.ltwPhones);
    listItem->setData(Qt::UserRole,phones.value(phone));
    ui.ltwPhones->addItem(listItem);
  }
  ui.tlbPhoneAdd->setVisible(!readOnly);
  ui.tlbPhoneDelete->setVisible(!readOnly);

  setLogo(QPixmap::fromImage(FVCard->logoImage()));
  ui.tlbLogoClear->setVisible(!readOnly);
  ui.tlbLogoLoad->setVisible(!readOnly);

  setPhoto(QPixmap::fromImage(FVCard->photoImage()));
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
  
  FVCard->setValueForTags(VVN_BIRTHDAY,ui.dedBirthday->date().toString(Qt::ISODate));
  FVCard->setValueForTags(VVN_GENDER,ui.cmbGender->currentText());
  FVCard->setValueForTags(VVN_MARITALSTATUS,ui.lneMaritial->text());
  FVCard->setValueForTags(VVN_TITLE,ui.lneTitle->text());
  FVCard->setValueForTags(VVN_ORG_UNIT,ui.lneDepartment->text());
  FVCard->setValueForTags(VVN_ORG_NAME,ui.lneCompany->text());
  FVCard->setValueForTags(VVN_ROLE,ui.lneRole->text());
  FVCard->setValueForTags(VVN_URL,ui.lneHomePage->text());

  static QStringList adresTags = QStringList() << "WORK" << "HOME";
  static QStringList tagHome = QStringList() << "HOME";
  static QStringList tagWork = QStringList() << "WORK";
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

  static QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
  for (int i = 0; i<ui.ltwEmails->count(); i++)
  {
    QListWidgetItem *listItem = ui.ltwEmails->item(i);
    FVCard->setTagsForValue(VVN_EMAIL,listItem->text(),listItem->data(Qt::UserRole).toStringList(),emailTagList);
  }

  static QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
  for (int i = 0; i<ui.ltwPhones->count(); i++)
  {
    QListWidgetItem *listItem = ui.ltwPhones->item(i);
    FVCard->setTagsForValue(VVN_TELEPHONE,listItem->text(),listItem->data(Qt::UserRole).toStringList(),phoneTagList);
  }
  
  if (!FLogo.isNull())
    FVCard->setLogoImage(FLogo.toImage());
  if (!FPhoto.isNull())
    FVCard->setPhotoImage(FPhoto.toImage());

  FVCard->setValueForTags(VVN_DESCRIPTION,ui.tedComments->toPlainText());
}

void VCardDialog::setPhoto(const QPixmap &APhoto)
{
  FPhoto = APhoto;
  if (!FPhoto.isNull())
    updatePhotoLabel(ui.lblPhoto->size());
  else
    ui.lblPhoto->clear();
  ui.tlbPhotoSave->setVisible(!FPhoto.isNull());
}

void VCardDialog::setLogo(const QPixmap &ALogo)
{
  FLogo = ALogo;
  if (!FLogo.isNull())
    updateLogoLabel(ui.lblLogo->size());
  else
    ui.lblLogo->clear();
  ui.tlbLogoSave->setVisible(!FLogo.isNull());
}

void VCardDialog::updatePhotoLabel(const QSize &ASize)
{
  if (!FPhoto.isNull())
    ui.lblPhoto->setPixmap(FPhoto.scaled(ASize-QSize(3,3),Qt::KeepAspectRatio));
}

void VCardDialog::updateLogoLabel(const QSize &ASize)
{
  if (!FLogo.isNull())
    ui.lblLogo->setPixmap(FLogo.scaled(ASize-QSize(3,3),Qt::KeepAspectRatio));
}

bool VCardDialog::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AEvent->type() == QEvent::Resize)
  {
    QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(AEvent);
    if (AObject == ui.lblPhoto)
      updatePhotoLabel(resizeEvent->size());
    else if(AObject == ui.lblLogo)
      updateLogoLabel(resizeEvent->size());
  }
  return QDialog::eventFilter(AObject,AEvent);
}

void VCardDialog::onVCardUpdated()
{
  ui.pbtUpdate->setEnabled(true);
  ui.pbtPublish->setEnabled(true);
  ui.pbtClear->setEnabled(true);
  ui.twtVCard->setEnabled(true);
  updateDialog();
}

void VCardDialog::onVCardPublished()
{
  ui.pbtUpdate->setEnabled(true);
  ui.pbtPublish->setEnabled(true);
  ui.pbtClear->setEnabled(true);
  ui.twtVCard->setEnabled(true);
}

void VCardDialog::onVCardError(const QString &AError)
{
  QMessageBox::critical(this,tr("vCard error"),tr("vCard request or publish failed.<br>%1").arg(AError));
  ui.pbtUpdate->setEnabled(true);
  ui.pbtPublish->setEnabled(true);
  ui.pbtClear->setEnabled(true);
  ui.twtVCard->setEnabled(true);
}

void VCardDialog::onUpdateClicked()
{
  reloadVCard();
}

void VCardDialog::onPublishClicked()
{
  updateVCard();
  publishVCard();
}

void VCardDialog::onClearClicked()
{
  FVCard->clear();
  updateDialog();
}

void VCardDialog::onCloseClicked()
{
  close();
}

void VCardDialog::onPhotoSaveClicked()
{
  if (!FPhoto.isNull())
  {
    QString filename = QFileDialog::getSaveFileName(this,tr("Save image"),"",tr("Image Files (*.png *.jpg *.bmp)"));
    if (!filename.isEmpty())
      FPhoto.save(filename);
  }
}

void VCardDialog::onPhotoLoadClicked()
{
  QString filename = QFileDialog::getOpenFileName(this,tr("Open image"),"",tr("Image Files (*.png *.jpg *.bmp)"));
  if (!filename.isEmpty())
  {
    QImage image(filename);
    if (!image.isNull())
      setPhoto(QPixmap::fromImage(image));
  }
}

void VCardDialog::onPhotoClearClicked()
{
  setPhoto(QPixmap());
}

void VCardDialog::onLogoSaveClicked()
{
  if (!FLogo.isNull())
  {
    QString filename = QFileDialog::getSaveFileName(this,tr("Save image"),"",tr("Image Files (*.png *.jpg *.bmp)"));
    if (!filename.isEmpty())
      FLogo.save(filename);
  }
}

void VCardDialog::onLogoLoadClicked()
{
  QString filename = QFileDialog::getOpenFileName(this,tr("Open image"),"",tr("Image Files (*.png *.jpg *.bmp)"));
  if (!filename.isEmpty())
  {
    QImage image(filename);
    if (!image.isNull())
      setLogo(QPixmap::fromImage(image));
  }
}

void VCardDialog::onLogoClearClicked()
{
  setLogo(QPixmap());
}

void VCardDialog::onEmailAddClicked()
{
  static QStringList emailTagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400";
  EditItemDialog dialog("",QStringList(),emailTagList,this);
  dialog.setLabelText(tr("EMail:"));
  if (dialog.exec() == QDialog::Accepted && !dialog.value().isEmpty() 
    && ui.ltwEmails->findItems(dialog.value(),Qt::MatchFixedString).isEmpty())
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

void VCardDialog::onEmailItemActivated(QListWidgetItem *AItem)
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

void VCardDialog::onPhoneAddClicked()
{
  static QStringList phoneTagList = QStringList() << "HOME" << "WORK" << "CELL" << "MODEM";
  EditItemDialog dialog("",QStringList(),phoneTagList,this);
  dialog.setLabelText(tr("Phone:"));
  if (dialog.exec() == QDialog::Accepted && !dialog.value().isEmpty() 
    && ui.ltwPhones->findItems(dialog.value(),Qt::MatchFixedString).isEmpty())
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

void VCardDialog::onPhoneItemActivated(QListWidgetItem *AItem)
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

