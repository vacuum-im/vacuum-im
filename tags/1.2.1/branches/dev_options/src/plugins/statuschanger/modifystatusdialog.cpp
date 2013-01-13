#include "modifystatusdialog.h"

ModifyStatusDialog::ModifyStatusDialog(IStatusChanger *AStatusChanger, int AStatusId, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose, true);
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SCHANGER_MODIFY_STATUS,0,0,"windowIcon");

  FStatusChanger = AStatusChanger;
  FStatusId = AStatusId;
  FStreamJid = AStreamJid;

  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::Online),FStatusChanger->nameByShow(IPresence::Online),IPresence::Online);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::Chat),FStatusChanger->nameByShow(IPresence::Chat),IPresence::Chat);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::Away),FStatusChanger->nameByShow(IPresence::Away),IPresence::Away);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::DoNotDisturb),FStatusChanger->nameByShow(IPresence::DoNotDisturb),IPresence::DoNotDisturb);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::ExtendedAway),FStatusChanger->nameByShow(IPresence::ExtendedAway),IPresence::ExtendedAway);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::Invisible),FStatusChanger->nameByShow(IPresence::Invisible),IPresence::Invisible);
  ui.cmbShow->addItem(FStatusChanger->iconByShow(IPresence::Offline),FStatusChanger->nameByShow(IPresence::Offline),IPresence::Offline);

  ui.cmbShow->setCurrentIndex(ui.cmbShow->findData(FStatusChanger->statusItemShow(FStatusId)));
  ui.cmbShow->setEnabled(AStatusId > STATUS_MAX_STANDART_ID);
  ui.lneName->setText(FStatusChanger->statusItemName(FStatusId));
  ui.spbPriority->setValue(FStatusChanger->statusItemPriority(FStatusId));
  ui.pteText->setPlainText(FStatusChanger->statusItemText(FStatusId));
  ui.pteText->setFocus();

  connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonBoxClicked(QAbstractButton *)));
}

ModifyStatusDialog::~ModifyStatusDialog()
{

}

void ModifyStatusDialog::modifyStatus()
{
  int show = ui.cmbShow->itemData(ui.cmbShow->currentIndex()).toInt();
  QString name = ui.lneName->text();
  int priority = ui.spbPriority->value();
  QString text = ui.pteText->toPlainText();
  
  if (  FStatusChanger->statusItemShow(FStatusId)!=show         || FStatusChanger->statusItemName(FStatusId)!=name ||
        FStatusChanger->statusItemPriority(FStatusId)!=priority || FStatusChanger->statusItemText(FStatusId)!=text)
  {
    FStatusChanger->updateStatusItem(FStatusId,name,show,text,priority);
    if (FStatusChanger->streamStatus(FStreamJid) != FStatusId)
      FStatusChanger->setStreamStatus(FStreamJid, FStatusId);
  }
  else
    FStatusChanger->setStreamStatus(FStreamJid, FStatusId);
}

void ModifyStatusDialog::onDialogButtonBoxClicked(QAbstractButton *AButton)
{
  switch (ui.dbbButtons->standardButton(AButton))
  {
  case QDialogButtonBox::Ok:
    modifyStatus();
    accept();
    break;
  default:
    reject();
    break;
  }
}

