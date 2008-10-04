#include "archiveoptions.h"

#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QIntValidator>

#define ONE_DAY           24*60*60
#define ONE_MONTH         ONE_DAY*31
#define ONE_YEAR          ONE_DAY*365

#define JID_COLUMN        0
#define OTR_COLUMN        1
#define SAVE_COLUMN       2
#define EXPIRE_COLUMN     3

ArchiveOptions::ArchiveOptions(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FArchiver = AArchiver;
  FStreamJid = AStreamJid;

  ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
  ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
  ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

  ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
  ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
  ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

  ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
  ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
  ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_APPROVE),ARCHIVE_OTR_APPROVE);
  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_CONCEDE),ARCHIVE_OTR_CONCEDE);
  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_FORBID),ARCHIVE_OTR_FORBID);
  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_OPPOSE),ARCHIVE_OTR_OPPOSE);
  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_PREFER),ARCHIVE_OTR_PREFER);
  ui.cmbModeOTR->addItem(AArchiver->otrModeName(ARCHIVE_OTR_REQUIRE),ARCHIVE_OTR_REQUIRE);

  ui.cmbModeSave->addItem(AArchiver->saveModeName(ARCHIVE_SAVE_FALSE),ARCHIVE_SAVE_FALSE);
  ui.cmbModeSave->addItem(AArchiver->saveModeName(ARCHIVE_SAVE_BODY),ARCHIVE_SAVE_BODY);
  ui.cmbModeSave->addItem(AArchiver->saveModeName(ARCHIVE_SAVE_MESSAGE),ARCHIVE_SAVE_MESSAGE);

  ui.cmbExpireTime->addItem(AArchiver->expireName(ONE_DAY),ONE_DAY);
  ui.cmbExpireTime->addItem(AArchiver->expireName(ONE_MONTH),ONE_MONTH);
  ui.cmbExpireTime->addItem(AArchiver->expireName(ONE_YEAR),ONE_YEAR);
  ui.cmbExpireTime->addItem(AArchiver->expireName(5*ONE_YEAR),5*ONE_YEAR);
  ui.cmbExpireTime->addItem(AArchiver->expireName(10*ONE_YEAR),10*ONE_YEAR);
  ui.cmbExpireTime->lineEdit()->setValidator(new QIntValidator(1,50*ONE_YEAR,ui.cmbExpireTime->lineEdit()));

  ui.tbwItemPrefs->setHorizontalHeaderLabels(QStringList()<<tr("JID")<<tr("OTR")<<tr("Save")<<tr("Expire"));
  ui.tbwItemPrefs->horizontalHeader()->setResizeMode(JID_COLUMN,QHeaderView::Stretch);
  ui.tbwItemPrefs->horizontalHeader()->setResizeMode(OTR_COLUMN,QHeaderView::ResizeToContents);
  ui.tbwItemPrefs->horizontalHeader()->setResizeMode(SAVE_COLUMN,QHeaderView::ResizeToContents);
  ui.tbwItemPrefs->horizontalHeader()->setResizeMode(EXPIRE_COLUMN,QHeaderView::ResizeToContents);
  
  reset();

  connect(FArchiver->instance(),SIGNAL(archiveAutoSaveChanged(const Jid &, bool)),
    SLOT(onArchiveAutoSaveChanged(const Jid &, bool)));
  connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)),
    SLOT(onArchivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)));
  connect(FArchiver->instance(),SIGNAL(archiveItemPrefsChanged(const Jid &, const Jid &, const IArchiveItemPrefs &)),
    SLOT(onArchiveItemPrefsChanged(const Jid &, const Jid &, const IArchiveItemPrefs &)));
  connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),
    SLOT(onArchiveRequestCompleted(const QString &)));
  connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
    SLOT(onArchiveRequestFailed(const QString &, const QString &)));
}

ArchiveOptions::~ArchiveOptions()
{

}

void ArchiveOptions::apply()
{
  if (FSaveRequests.isEmpty())
  {
    IArchiveStreamPrefs prefs;
    prefs.methodLocal = ui.cmbMethodLocal->itemData(ui.cmbMethodLocal->currentIndex()).toString();
    prefs.methodAuto = ui.cmbMethodAuto->itemData(ui.cmbMethodAuto->currentIndex()).toString();
    prefs.methodManual = ui.cmbMethodManual->itemData(ui.cmbMethodManual->currentIndex()).toString();
    prefs.defaultPrefs.otr = ui.cmbModeOTR->itemData(ui.cmbModeOTR->currentIndex()).toString();
    prefs.defaultPrefs.save = ui.cmbModeSave->itemData(ui.cmbModeSave->currentIndex()).toString();
    int expireIndex = ui.cmbExpireTime->findText(ui.cmbExpireTime->currentText());
    if (expireIndex >= 0)
      prefs.defaultPrefs.expire = ui.cmbExpireTime->itemData(expireIndex).toInt();
    else
      prefs.defaultPrefs.expire = ui.cmbExpireTime->currentText().toInt()*ONE_DAY;

    QString requestId = FArchiver->setArchivePrefs(FStreamJid,prefs);
    if (!requestId.isEmpty())
    {
      ui.grbMethod->setEnabled(false);
      ui.grbDefault->setEnabled(false);
      ui.grbIndividual->setEnabled(false);
      FSaveRequests.append(requestId);
      ui.lblStatus->setText(tr("Waiting for host response..."));
    }
  }
}

void ArchiveOptions::reset()
{
  FTableItems.clear();
  ui.tbwItemPrefs->clearContents();
  if (FArchiver->isReady(FStreamJid))
  {
    IArchiveStreamPrefs prefs = FArchiver->archivePrefs(FStreamJid);
    QHash<Jid,IArchiveItemPrefs>::const_iterator it = prefs.itemPrefs.constBegin();
    while (it!=prefs.itemPrefs.constEnd())
    {
      onArchiveItemPrefsChanged(FStreamJid,it.key(),it.value());
      it++;
    }
    onArchivePrefsChanged(FStreamJid,prefs);
  }
  else
  {
    ui.grbMethod->setEnabled(false);
    ui.grbDefault->setEnabled(false);
    ui.grbIndividual->setEnabled(false);
    ui.lblStatus->setText(tr("Preferences not loaded"));
  }
}

void ArchiveOptions::onArchiveAutoSaveChanged(const Jid &AStreamJid, bool AAutoSave)
{
  if (AStreamJid == FStreamJid)
  {
    ui.chbAutoSave->setChecked(AAutoSave);
  }
}

void ArchiveOptions::onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs)
{
  if (AStreamJid == FStreamJid)
  {
    onArchiveAutoSaveChanged(AStreamJid,APrefs.autoSave);

    ui.cmbMethodLocal->setCurrentIndex(ui.cmbMethodLocal->findData(APrefs.methodLocal));
    ui.cmbMethodAuto->setCurrentIndex(ui.cmbMethodAuto->findData(APrefs.methodAuto));
    ui.cmbMethodManual->setCurrentIndex(ui.cmbMethodManual->findData(APrefs.methodManual));
    ui.grbMethod->setEnabled(true);

    ui.cmbModeOTR->setCurrentIndex(ui.cmbModeOTR->findData(APrefs.defaultPrefs.otr));
    ui.cmbModeSave->setCurrentIndex(ui.cmbModeSave->findData(APrefs.defaultPrefs.save));
    int expireIndex = ui.cmbExpireTime->findData(APrefs.defaultPrefs.expire);
    if (expireIndex<0)
    {
      ui.cmbExpireTime->addItem(FArchiver->expireName(APrefs.defaultPrefs.expire),APrefs.defaultPrefs.expire);
      expireIndex = ui.cmbExpireTime->findData(APrefs.defaultPrefs.expire);
    }
    ui.cmbExpireTime->setCurrentIndex(expireIndex);
    ui.grbDefault->setEnabled(true);

    ui.grbIndividual->setEnabled(true);
    ui.lblStatus->clear();

    ui.grbMethod->setVisible(FArchiver->isSupported(FStreamJid));
  }
}

void ArchiveOptions::onArchiveItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IArchiveItemPrefs &APrefs)
{
  if (AStreamJid == FStreamJid)
  {
    if (!FTableItems.contains(AItemJid))
    {
      QTableWidgetItem *jidItem = new QTableWidgetItem(AItemJid.full());
      QTableWidgetItem *otrItem = new QTableWidgetItem();
      QTableWidgetItem *saveItem = new QTableWidgetItem();
      QTableWidgetItem *expireItem = new QTableWidgetItem();
      ui.tbwItemPrefs->setRowCount(ui.tbwItemPrefs->rowCount()+1);
      ui.tbwItemPrefs->setItem(ui.tbwItemPrefs->rowCount()-1,JID_COLUMN,jidItem);
      ui.tbwItemPrefs->setItem(jidItem->row(),OTR_COLUMN,otrItem);
      ui.tbwItemPrefs->setItem(jidItem->row(),SAVE_COLUMN,saveItem);
      ui.tbwItemPrefs->setItem(jidItem->row(),EXPIRE_COLUMN,expireItem);
      FTableItems.insert(AItemJid,jidItem);
    }
    QTableWidgetItem *jidItem = FTableItems.value(AItemJid);
    ui.tbwItemPrefs->item(jidItem->row(),OTR_COLUMN)->setText(FArchiver->otrModeName(APrefs.otr));
    ui.tbwItemPrefs->item(jidItem->row(),SAVE_COLUMN)->setText(FArchiver->saveModeName(APrefs.save));
    ui.tbwItemPrefs->item(jidItem->row(),EXPIRE_COLUMN)->setText(FArchiver->expireName(APrefs.expire));
  }
}

void ArchiveOptions::onArchiveRequestCompleted(const QString &AId)
{
  if (FSaveRequests.contains(AId))
  {
    ui.lblStatus->setText(tr("Preferences accepted"));
    FSaveRequests.removeAt(FSaveRequests.indexOf(AId));
  }
}

void ArchiveOptions::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
  if (FSaveRequests.contains(AId))
  {
    reset();
    ui.lblStatus->setText(tr("Preferences not accepted: %1").arg(AError));
    FSaveRequests.removeAt(FSaveRequests.indexOf(AId));
  }
}

