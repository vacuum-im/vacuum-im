#include "addcontactdialog.h"

#include <QMessageBox>

AddContactDialog::AddContactDialog(QWidget *AParent)
  : QDialog(AParent)
{
  setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setRequestText(tr("Please, authorize me to your presence."));
  connect(btbButtons,SIGNAL(accepted()),SLOT(onAccepted()));
}

AddContactDialog::~AddContactDialog()
{

}

const Jid &AddContactDialog::stramJid() const
{
  return FStreamJid;
}

Jid AddContactDialog::contactJid() const
{
  return lneJabberId->text();
}

QString AddContactDialog::nick() const
{
  return lneNickName->text();
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

QString AddContactDialog::requestText() const
{
  return tedRequest->toPlainText();
}

bool AddContactDialog::requestSubscr() const
{
  return chbRequestSubscr->checkState() == Qt::Checked;
}

bool AddContactDialog::sendSubscr() const
{
  return chbSendSubscr->checkState() == Qt::Checked;
}

void AddContactDialog::setStreamJid( const Jid &AStreamJid )
{
  FStreamJid = AStreamJid;
  setWindowTitle(tr("Add contact - %1").arg(FStreamJid.full()));
}

void AddContactDialog::setContactJid(const Jid &AJid)
{
  lneJabberId->setText(AJid.bare());
  if (nick().isEmpty())
    setNick(AJid.node());
}

void AddContactDialog::setNick(const QString &ANick)
{
  if (!ANick.isEmpty())
    lneNickName->setText(ANick);
  else
    lneNickName->setText(contactJid().node());
}

void AddContactDialog::setGroup(const QString &AGroup)
{
  if (!AGroup.isEmpty())
    cmbGroup->setEditText(AGroup);
  else
    cmbGroup->setCurrentIndex(0);
}

void AddContactDialog::setGroups(const QSet<QString> &AGroups)
{
  QStringList grps = AGroups.toList();
  grps.sort();
  cmbGroup->addItems(QStringList()<<tr("<None>")<<grps);
}

void AddContactDialog::setRequestText(const QString &AText)
{
  tedRequest->setPlainText(AText);
}

void AddContactDialog::onAccepted()
{
  Jid cJid = contactJid();
  if (cJid.isValid() && !FStreamJid.equals(cJid,false))
  {
    emit addContact(this);
    accept();
  }
  else
    QMessageBox::warning(this,FStreamJid.full(),
      tr("Can`t add contact '<b>%1</b>'<br>'<b>%1</b>' is not a valid Jaber ID").arg(cJid.bare()));
}


