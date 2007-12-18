#include "addcontactdialog.h"

#include <QMessageBox>

AddContactDialog::AddContactDialog(const Jid &AStreamJid, const QSet<QString> &AGroups, QWidget *AParent) : QDialog(AParent)
{
  setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  
  FStreamJid = AStreamJid;
  setWindowTitle(tr("Add contact to %1").arg(FStreamJid.bare()));

  QStringList grps = AGroups.toList();
  grps.sort();
  cmbGroup->addItems(QStringList()<<tr("<None>")<<grps);

  setRequestText(tr("Please, authorize me to your presence."));
  connect(btbButtons,SIGNAL(accepted()),SLOT(onAccepted()));
}

AddContactDialog::~AddContactDialog()
{

}

const Jid &AddContactDialog::streamJid() const
{
  return FStreamJid;
}

Jid AddContactDialog::contactJid() const
{
  return lneJabberId->text();
}

void AddContactDialog::setContactJid(const Jid &AJid)
{
  lneJabberId->setText(AJid.hBare());
  if (nick().isEmpty())
    setNick(AJid.hNode());
}

QString AddContactDialog::nick() const
{
  return lneNickName->text();
}

void AddContactDialog::setNick(const QString &ANick)
{
  if (!ANick.isEmpty())
    lneNickName->setText(ANick);
  else
    lneNickName->setText(contactJid().hNode());
}

QSet<QString> AddContactDialog::groups() const
{
  QSet<QString> grps;
  int curIndex = cmbGroup->currentIndex();
  QString curText = cmbGroup->currentText();
  if (curIndex > 0 || (!curText.isEmpty() && curText!=cmbGroup->itemText(0)))
    grps.insert(cmbGroup->currentText());
  return grps;
}

void AddContactDialog::setGroup(const QString &AGroup)
{
  if (!AGroup.isEmpty())
    cmbGroup->setEditText(AGroup);
  else
    cmbGroup->setCurrentIndex(0);
}

QString AddContactDialog::requestText() const
{
  return tedRequest->toPlainText();
}

void AddContactDialog::setRequestText(const QString &AText)
{
  tedRequest->setPlainText(AText);
}

bool AddContactDialog::requestSubscription() const
{
  return chbRequestSubscr->checkState() == Qt::Checked;
}

bool AddContactDialog::sendSubscription() const
{
  return chbSendSubscr->checkState() == Qt::Checked;
}

void AddContactDialog::onAccepted()
{
  Jid cJid = contactJid();
  if (cJid.isValid() && !(FStreamJid && cJid))
  {
    emit addContact(this);
    accept();
  }
  else
    QMessageBox::warning(this,FStreamJid.bare(),
      tr("Can`t add contact '<b>%1</b>'<br>'<b>%1</b>' is not a valid Jaber ID").arg(cJid.hBare()));
}


