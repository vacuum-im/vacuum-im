#include "archiveoptions.h"

#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QIntValidator>
#include <QTextDocument>

#define ONE_DAY           (24*60*60)
#define ONE_MONTH         (ONE_DAY*31)
#define ONE_YEAR          (ONE_DAY*365)

#define JID_COLUMN        0
#define OTR_COLUMN        1
#define SAVE_COLUMN       2
#define EXPIRE_COLUMN     3

ArchiveDelegate::ArchiveDelegate(IMessageArchiver *AArchiver, QObject *AParent) : QItemDelegate(AParent)
{
	FArchiver = AArchiver;
}

QWidget *ArchiveDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &/*AOption*/, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case OTR_COLUMN:
	case SAVE_COLUMN:
	case EXPIRE_COLUMN:
		QComboBox *comboBox = new QComboBox(AParent);
		updateComboBox(AIndex.column(),comboBox);
		return comboBox;
	}
	return NULL;
}

void ArchiveDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
	int index = comboBox->findData(AIndex.data(Qt::UserRole));
	switch (AIndex.column())
	{
	case OTR_COLUMN:
	case SAVE_COLUMN:
		comboBox->setCurrentIndex(index);
		break;
	case EXPIRE_COLUMN:
		comboBox->lineEdit()->setText(QString::number(AIndex.data(Qt::UserRole).toInt()/ONE_DAY));
	default:
		break;
	}
}

void ArchiveDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
	int index = comboBox->currentIndex();
	switch (AIndex.column())
	{
	case OTR_COLUMN:
	case SAVE_COLUMN:
		AModel->setData(AIndex,comboBox->itemText(index),Qt::DisplayRole);
		AModel->setData(AIndex,comboBox->itemData(index),Qt::UserRole);
		break;
	case EXPIRE_COLUMN:
	{
		int expire = comboBox->currentText().toInt()*ONE_DAY;
		AModel->setData(AIndex,FArchiver->expireName(expire),Qt::DisplayRole);
		AModel->setData(AIndex,expire,Qt::UserRole);
	}
	break;
	default:
		break;
	}
}

void ArchiveDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
	int widthDelta = AEditor->sizeHint().width() - AEditor->width();
	if (widthDelta>0)
		AEditor->setGeometry(AEditor->x()-widthDelta,AEditor->y(),AEditor->width()+widthDelta,AEditor->height());
}

void ArchiveDelegate::updateComboBox(int AColumn, QComboBox *AComboBox) const
{
	switch (AColumn)
	{
	case OTR_COLUMN:
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_APPROVE),ARCHIVE_OTR_APPROVE);
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_CONCEDE),ARCHIVE_OTR_CONCEDE);
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_FORBID),ARCHIVE_OTR_FORBID);
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_OPPOSE),ARCHIVE_OTR_OPPOSE);
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_PREFER),ARCHIVE_OTR_PREFER);
		AComboBox->addItem(FArchiver->otrModeName(ARCHIVE_OTR_REQUIRE),ARCHIVE_OTR_REQUIRE);
		break;
	case SAVE_COLUMN:
		AComboBox->addItem(FArchiver->saveModeName(ARCHIVE_SAVE_FALSE),ARCHIVE_SAVE_FALSE);
		AComboBox->addItem(FArchiver->saveModeName(ARCHIVE_SAVE_BODY),ARCHIVE_SAVE_BODY);
		AComboBox->addItem(FArchiver->saveModeName(ARCHIVE_SAVE_MESSAGE),ARCHIVE_SAVE_MESSAGE);
		break;
	case EXPIRE_COLUMN:
		AComboBox->setEditable(true);
		AComboBox->addItem(FArchiver->expireName(0),0);
		AComboBox->addItem(FArchiver->expireName(ONE_DAY),ONE_DAY);
		AComboBox->addItem(FArchiver->expireName(ONE_MONTH),ONE_MONTH);
		AComboBox->addItem(FArchiver->expireName(ONE_YEAR),ONE_YEAR);
		AComboBox->addItem(FArchiver->expireName(5*ONE_YEAR),5*ONE_YEAR);
		AComboBox->addItem(FArchiver->expireName(10*ONE_YEAR),10*ONE_YEAR);
		AComboBox->lineEdit()->setValidator(new QIntValidator(0,50*ONE_YEAR,AComboBox->lineEdit()));
		connect(AComboBox,SIGNAL(currentIndexChanged(int)),SLOT(onExpireIndexChanged(int)));
		break;
	default:
		break;
	}
}

void ArchiveDelegate::onExpireIndexChanged(int AIndex)
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
	comboBox->lineEdit()->setText(QString::number(comboBox->itemData(AIndex).toInt()/ONE_DAY));
}

ArchiveOptions::ArchiveOptions(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FArchiver = AArchiver;
	FStreamJid = AStreamJid;

	ArchiveDelegate *delegat = new ArchiveDelegate(AArchiver,ui.tbwItemPrefs);
	ui.tbwItemPrefs->setItemDelegate(delegat);
	ui.tbwItemPrefs->verticalHeader()->hide();
	ui.tbwItemPrefs->setHorizontalHeaderLabels(QStringList()<<tr("JID")<<tr("OTR")<<tr("Save")<<tr("Expire"));
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(JID_COLUMN,QHeaderView::Stretch);
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(OTR_COLUMN,QHeaderView::ResizeToContents);
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(SAVE_COLUMN,QHeaderView::ResizeToContents);
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(EXPIRE_COLUMN,QHeaderView::ResizeToContents);

	ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
	ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
	ui.cmbMethodLocal->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

	ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
	ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
	ui.cmbMethodAuto->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

	ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_PREFER),ARCHIVE_METHOD_PREFER);
	ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_CONCEDE),ARCHIVE_METHOD_CONCEDE);
	ui.cmbMethodManual->addItem(AArchiver->methodName(ARCHIVE_METHOD_FORBID),ARCHIVE_METHOD_FORBID);

	delegat->updateComboBox(OTR_COLUMN,ui.cmbModeOTR);
	delegat->updateComboBox(SAVE_COLUMN,ui.cmbModeSave);
	delegat->updateComboBox(EXPIRE_COLUMN,ui.cmbExpireTime);

	reset();

	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddItemPrefClicked()));
	connect(ui.pbtRemove,SIGNAL(clicked()),SLOT(onRemoveItemPrefClicked()));
	connect(FArchiver->instance(),SIGNAL(archiveAutoSaveChanged(const Jid &, bool)),
	        SLOT(onArchiveAutoSaveChanged(const Jid &, bool)));
	connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)),
	        SLOT(onArchivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)));
	connect(FArchiver->instance(),SIGNAL(archiveItemPrefsChanged(const Jid &, const Jid &, const IArchiveItemPrefs &)),
	        SLOT(onArchiveItemPrefsChanged(const Jid &, const Jid &, const IArchiveItemPrefs &)));
	connect(FArchiver->instance(),SIGNAL(archiveItemPrefsRemoved(const Jid &, const Jid &)),
	        SLOT(onArchiveItemPrefsRemoved(const Jid &, const Jid &)));
	connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),
	        SLOT(onArchiveRequestCompleted(const QString &)));
	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
	        SLOT(onArchiveRequestFailed(const QString &, const QString &)));

	connect(ui.cmbMethodLocal,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbMethodManual,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbMethodAuto,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbModeOTR,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbModeSave,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbExpireTime,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.cmbExpireTime->lineEdit(),SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.chbAutoSave,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbReplication,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(delegat,SIGNAL(commitData(QWidget *)),SIGNAL(modified()));
}

ArchiveOptions::~ArchiveOptions()
{

}

void ArchiveOptions::apply()
{
	if (FSaveRequests.isEmpty())
	{
		IArchiveStreamPrefs prefs = FArchiver->archivePrefs(FStreamJid);
		prefs.methodLocal = ui.cmbMethodLocal->itemData(ui.cmbMethodLocal->currentIndex()).toString();
		prefs.methodAuto = ui.cmbMethodAuto->itemData(ui.cmbMethodAuto->currentIndex()).toString();
		prefs.methodManual = ui.cmbMethodManual->itemData(ui.cmbMethodManual->currentIndex()).toString();
		prefs.defaultPrefs.otr = ui.cmbModeOTR->itemData(ui.cmbModeOTR->currentIndex()).toString();
		prefs.defaultPrefs.save = ui.cmbModeSave->itemData(ui.cmbModeSave->currentIndex()).toString();
		prefs.defaultPrefs.expire = ui.cmbExpireTime->currentText().toInt()*ONE_DAY;

		foreach(Jid itemJid, FTableItems.keys())
		{
			QTableWidgetItem *jidItem = FTableItems.value(itemJid);
			prefs.itemPrefs[itemJid].otr = ui.tbwItemPrefs->item(jidItem->row(),OTR_COLUMN)->data(Qt::UserRole).toString();
			prefs.itemPrefs[itemJid].save = ui.tbwItemPrefs->item(jidItem->row(),SAVE_COLUMN)->data(Qt::UserRole).toString();
			prefs.itemPrefs[itemJid].expire = ui.tbwItemPrefs->item(jidItem->row(),EXPIRE_COLUMN)->data(Qt::UserRole).toInt();
		}

		foreach(Jid itemJid, prefs.itemPrefs.keys())
		{
			if (!FTableItems.contains(itemJid))
			{
				if (FArchiver->isSupported(FStreamJid))
				{
					QString requestId = FArchiver->removeArchiveItemPrefs(FStreamJid,itemJid);
					if (!requestId.isEmpty())
						FSaveRequests.append(requestId);
					prefs.itemPrefs.remove(itemJid);
				}
				else
				{
					prefs.itemPrefs[itemJid].otr = "";
					prefs.itemPrefs[itemJid].save = "";
				}
			}
		}

		QString requestId = FArchiver->setArchivePrefs(FStreamJid,prefs);
		if (!requestId.isEmpty())
			FSaveRequests.append(requestId);

		if (prefs.autoSave != ui.chbAutoSave->isChecked())
		{
			requestId = FArchiver->setArchiveAutoSave(FStreamJid,ui.chbAutoSave->isChecked());
			if (!requestId.isEmpty())
				FSaveRequests.append(requestId);
		}

		if (FArchiver->isReplicationEnabled(FStreamJid) != ui.chbReplication->isChecked())
			FArchiver->setReplicationEnabled(FStreamJid,ui.chbReplication->isChecked());

		FLastError.clear();
		updateWidget();
	}
	emit childApply();
}

void ArchiveOptions::reset()
{
	FTableItems.clear();
	ui.tbwItemPrefs->clearContents();
	ui.tbwItemPrefs->setRowCount(0);
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
		ui.chbReplication->setCheckState(FArchiver->isReplicationEnabled(FStreamJid) ? Qt::Checked : Qt::Unchecked);
		FLastError.clear();
	}
	else
	{
		FLastError = tr("History preferences not loaded");
	}
	updateWidget();
	emit childReset();
}

void ArchiveOptions::updateWidget()
{
	if (!FSaveRequests.isEmpty())
	{
		ui.grbMethod->setEnabled(false);
		ui.grbDefault->setEnabled(false);
		ui.grbIndividual->setEnabled(false);
		ui.lblStatus->setVisible(true);
		ui.lblStatus->setText(tr("Waiting for host response..."));
	}
	else
	{
		ui.grbMethod->setEnabled(true);
		ui.grbDefault->setEnabled(true);
		ui.grbIndividual->setEnabled(true);
		if (!FLastError.isEmpty())
			ui.lblStatus->setText(tr("Error received: %1").arg(Qt::escape(FLastError)));
		else
			ui.lblStatus->setVisible(false);
	}
}

void ArchiveOptions::updateColumnsSize()
{
	ui.tbwItemPrefs->horizontalHeader()->reset();
}

void ArchiveOptions::onAddItemPrefClicked()
{
	Jid itemJid = QInputDialog::getText(this,tr("New item preferences"),tr("Enter item JID:"));
	if (itemJid.isValid() && !FTableItems.contains(itemJid))
	{
		IArchiveItemPrefs itemPrefs = FArchiver->archiveItemPrefs(FStreamJid,itemJid);
		onArchiveItemPrefsChanged(FStreamJid,itemJid,itemPrefs);
		ui.tbwItemPrefs->setCurrentItem(FTableItems.value(itemJid));
		emit modified();
	}
	else if (!itemJid.isEmpty())
	{
		QMessageBox::warning(this,tr("Unacceptable item JID"),tr("'%1' is not valid JID or already exists").arg(itemJid.hFull()));
	}
}

void ArchiveOptions::onRemoveItemPrefClicked()
{
	if (ui.tbwItemPrefs->currentRow()>=0)
	{
		QTableWidgetItem *jidItem = ui.tbwItemPrefs->item(ui.tbwItemPrefs->currentRow(),JID_COLUMN);
		onArchiveItemPrefsRemoved(FStreamJid,FTableItems.key(jidItem));
		emit modified();
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
		ui.cmbExpireTime->lineEdit()->setText(QString::number(APrefs.defaultPrefs.expire/ONE_DAY));
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
			ui.tbwItemPrefs->verticalHeader()->setResizeMode(jidItem->row(),QHeaderView::ResizeToContents);
			FTableItems.insert(AItemJid,jidItem);
		}
		QTableWidgetItem *jidItem = FTableItems.value(AItemJid);
		ui.tbwItemPrefs->item(jidItem->row(),OTR_COLUMN)->setText(FArchiver->otrModeName(APrefs.otr));
		ui.tbwItemPrefs->item(jidItem->row(),OTR_COLUMN)->setData(Qt::UserRole,APrefs.otr);
		ui.tbwItemPrefs->item(jidItem->row(),SAVE_COLUMN)->setText(FArchiver->saveModeName(APrefs.save));
		ui.tbwItemPrefs->item(jidItem->row(),SAVE_COLUMN)->setData(Qt::UserRole,APrefs.save);
		ui.tbwItemPrefs->item(jidItem->row(),EXPIRE_COLUMN)->setText(FArchiver->expireName(APrefs.expire));
		ui.tbwItemPrefs->item(jidItem->row(),EXPIRE_COLUMN)->setData(Qt::UserRole,APrefs.expire);
		updateColumnsSize();
	}
}

void ArchiveOptions::onArchiveItemPrefsRemoved(const Jid &AStreamJid, const Jid &AItemJid)
{
	if (AStreamJid==FStreamJid && FTableItems.contains(AItemJid))
	{
		QTableWidgetItem *jidItem = FTableItems.take(AItemJid);
		ui.tbwItemPrefs->removeRow(jidItem->row());
		updateColumnsSize();
	}
}

void ArchiveOptions::onArchiveRequestCompleted(const QString &AId)
{
	if (FSaveRequests.contains(AId))
	{
		ui.lblStatus->setText(tr("Preferences accepted"));
		FSaveRequests.removeAt(FSaveRequests.indexOf(AId));
		updateWidget();
	}
}

void ArchiveOptions::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FSaveRequests.contains(AId))
	{
		FLastError = AError;
		FSaveRequests.removeAt(FSaveRequests.indexOf(AId));
		updateWidget();
	}
}
