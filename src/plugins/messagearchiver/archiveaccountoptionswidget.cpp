#include "archiveaccountoptionswidget.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>

enum Columns {
	COL_JID,
	COL_SAVE,
};

ArchiveDelegate::ArchiveDelegate(IMessageArchiver *AArchiver, QObject *AParent) : QItemDelegate(AParent)
{
	FArchiver = AArchiver;
}

QString ArchiveDelegate::saveModeName(const QString &ASaveMode)
{
	if (ASaveMode == ARCHIVE_SAVE_ROSTER)
		return tr("Roster");
	else if (ASaveMode == ARCHIVE_SAVE_ALWAYS)
		return tr("Always");
	else if (ASaveMode == ARCHIVE_SAVE_NEVER)
		return tr("Never");
	else
		return tr("Unknown");
}

void ArchiveDelegate::updateComboBox(int AColumn, QComboBox *AComboBox)
{
	switch (AColumn)
	{
	case COL_SAVE:
		AComboBox->addItem(saveModeName(ARCHIVE_SAVE_ALWAYS),ARCHIVE_SAVE_ALWAYS);
		AComboBox->addItem(saveModeName(ARCHIVE_SAVE_NEVER),ARCHIVE_SAVE_NEVER);
		break;
	default:
		break;
	}
}

QWidget *ArchiveDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(AOption);
	switch (AIndex.column())
	{
	case COL_SAVE:
		{
			QComboBox *comboBox = new QComboBox(AParent);
			updateComboBox(AIndex.column(),comboBox);
			return comboBox;
		}
	}
	return NULL;
}

void ArchiveDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_SAVE:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
				comboBox->setCurrentIndex(comboBox->findData(AIndex.data(Qt::UserRole)));
		}
		break;
	default:
		break;
	}
}

void ArchiveDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_SAVE:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
			{
				int index = comboBox->currentIndex();
				AModel->setData(AIndex,comboBox->itemText(index),Qt::DisplayRole);
				AModel->setData(AIndex,comboBox->itemData(index),Qt::UserRole);
			}
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


ArchiveAccountOptionsWidget::ArchiveAccountOptionsWidget(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FArchiver = AArchiver;
	FStreamJid = AStreamJid;

	ui.lblDefault->setText(QString("<h3>%1</h3>").arg(ui.lblDefault->text()));
	ui.lblEntity->setText(QString("<h3>%1</h3>").arg(ui.lblEntity->text()));

	ArchiveDelegate *delegat = new ArchiveDelegate(AArchiver,ui.tbwItemPrefs);
	ui.tbwItemPrefs->setItemDelegate(delegat);
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(COL_JID,QHeaderView::Stretch);
	ui.tbwItemPrefs->horizontalHeader()->setResizeMode(COL_SAVE,QHeaderView::ResizeToContents);

	ui.cmbDefaultSave->addItem(tr("Save messages from contacts in roster"),ARCHIVE_SAVE_ROSTER);
	ui.cmbDefaultSave->addItem(tr("Save all messages"),ARCHIVE_SAVE_ALWAYS);
	ui.cmbDefaultSave->addItem(tr("Do not save messages"),ARCHIVE_SAVE_NEVER);

	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddItemPrefClicked()));
	connect(ui.pbtRemove,SIGNAL(clicked()),SLOT(onRemoveItemPrefClicked()));
	connect(FArchiver->instance(),SIGNAL(archivePrefsOpened(const Jid &)),SLOT(onArchivePrefsOpened(const Jid &)));
	connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &)),SLOT(onArchivePrefsChanged(const Jid &)));
	connect(FArchiver->instance(),SIGNAL(archivePrefsClosed(const Jid &)),SLOT(onArchivePrefsClosed(const Jid &)));
	connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),SLOT(onArchiveRequestCompleted(const QString &)));
	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));

	connect(ui.cmbDefaultSave,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(delegat,SIGNAL(commitData(QWidget *)),SIGNAL(modified()));

	reset();
}

ArchiveAccountOptionsWidget::~ArchiveAccountOptionsWidget()
{

}

void ArchiveAccountOptionsWidget::apply()
{
	if (FSaveRequests.isEmpty())
	{
		IArchiveStreamPrefs prefs = FArchiver->archivePrefs(FStreamJid);
		prefs.defaults = ui.cmbDefaultSave->itemData(ui.cmbDefaultSave->currentIndex()).toString();

		prefs.never.clear();
		prefs.always.clear();
		foreach(const Jid &itemJid, FTableItems.keys())
		{
			QTableWidgetItem *jidItem = FTableItems.value(itemJid);
			QString save = ui.tbwItemPrefs->item(jidItem->row(),COL_SAVE)->data(Qt::UserRole).toString();
			if (save == ARCHIVE_SAVE_ALWAYS)
				prefs.always += itemJid;
			else if (save == ARCHIVE_SAVE_NEVER)
				prefs.never += itemJid;
		}

		QString requestId = FArchiver->setArchivePrefs(FStreamJid,prefs);
		if (!requestId.isEmpty())
			FSaveRequests.append(requestId);

		FLastError = XmppError::null;
		updateWidget();
	}

	emit childApply();
}

void ArchiveAccountOptionsWidget::reset()
{
	FTableItems.clear();
	ui.tbwItemPrefs->clearContents();
	ui.tbwItemPrefs->setRowCount(0);

	if (FArchiver->isReady(FStreamJid))
		onArchivePrefsChanged(FStreamJid);

	FLastError = XmppError::null;
	updateWidget();

	emit childReset();
}

void ArchiveAccountOptionsWidget::updateWidget()
{
	bool requesting = !FSaveRequests.isEmpty();

	ui.wdtDefault->setEnabled(!requesting);
	ui.wdtEntity->setEnabled(!requesting);

	if (requesting)
		ui.lblStatus->setText(tr("Waiting for host response..."));
	else if (!FArchiver->isPrefsSupported(FStreamJid))
		ui.lblStatus->setText(tr("History preferences is not available"));
	else if (!FLastError.isNull())
		ui.lblStatus->setText(tr("Failed to save archive preferences: %1").arg(FLastError.errorMessage()));
	else if (!ui.lblStatus->text().isEmpty())
		ui.lblStatus->setText(tr("Preferences accepted"));

	setEnabled(FArchiver->isPrefsSupported(FStreamJid));
}

void ArchiveAccountOptionsWidget::updateColumnsSize()
{
	ui.tbwItemPrefs->horizontalHeader()->reset();
}

void ArchiveAccountOptionsWidget::updateItemPrefs(const Jid &AItemJid, const QString &ASaveMode)
{
	if (!FTableItems.contains(AItemJid))
	{
		QTableWidgetItem *jidItem = new QTableWidgetItem(AItemJid.uFull());
		QTableWidgetItem *saveItem = new QTableWidgetItem();
		ui.tbwItemPrefs->setRowCount(ui.tbwItemPrefs->rowCount()+1);
		ui.tbwItemPrefs->setItem(ui.tbwItemPrefs->rowCount()-1,COL_JID,jidItem);
		ui.tbwItemPrefs->setItem(jidItem->row(),COL_SAVE,saveItem);
		ui.tbwItemPrefs->verticalHeader()->setResizeMode(jidItem->row(),QHeaderView::ResizeToContents);
		FTableItems.insert(AItemJid,jidItem);
	}

	QTableWidgetItem *jidItem = FTableItems.value(AItemJid);
	ui.tbwItemPrefs->item(jidItem->row(),COL_SAVE)->setText(ArchiveDelegate::saveModeName(ASaveMode));
	ui.tbwItemPrefs->item(jidItem->row(),COL_SAVE)->setData(Qt::UserRole,ASaveMode);
}

void ArchiveAccountOptionsWidget::removeItemPrefs(const Jid &AItemJid)
{
	if (FTableItems.contains(AItemJid))
	{
		QTableWidgetItem *jidItem = FTableItems.take(AItemJid);
		ui.tbwItemPrefs->removeRow(jidItem->row());
	}
}

void ArchiveAccountOptionsWidget::onAddItemPrefClicked()
{
	Jid itemJid = Jid::fromUserInput(QInputDialog::getText(this,tr("New item preferences"),tr("Enter item JID:")));
	if (itemJid.isValid() && !FTableItems.contains(itemJid))
	{
		updateItemPrefs(itemJid,ARCHIVE_SAVE_NEVER);
		ui.tbwItemPrefs->setCurrentItem(FTableItems.value(itemJid));
		emit modified();
	}
	else if (!itemJid.isEmpty())
	{
		QMessageBox::warning(this,tr("Unacceptable item JID"),tr("'%1' is not valid JID or already exists").arg(Qt::escape(itemJid.uFull())));
	}

	updateColumnsSize();
}

void ArchiveAccountOptionsWidget::onRemoveItemPrefClicked()
{
	QList<QTableWidgetItem *> selectedRows;
	foreach(QTableWidgetItem *item, ui.tbwItemPrefs->selectedItems())
		if (item->column() == COL_JID)
			selectedRows.append(item);

	foreach(QTableWidgetItem *item, selectedRows)
	{
		removeItemPrefs(FTableItems.key(item));
		emit modified();
	}

	updateColumnsSize();
}

void ArchiveAccountOptionsWidget::onArchivePrefsOpened( const Jid &AStreamJid )
{
	if (AStreamJid == FStreamJid)
		updateWidget();
}

void ArchiveAccountOptionsWidget::onArchivePrefsChanged(const Jid &AStreamJid)
{
	if (AStreamJid == FStreamJid)
	{
		QSet<Jid> oldItems = FTableItems.keys().toSet();

		IArchiveStreamPrefs prefs = FArchiver->archivePrefs(AStreamJid);
		ui.cmbDefaultSave->setCurrentIndex(ui.cmbDefaultSave->findData(prefs.defaults));

		foreach(const Jid &itemJid, prefs.never)
		{
			oldItems -= itemJid;
			updateItemPrefs(itemJid,ARCHIVE_SAVE_NEVER);
		}

		foreach(const Jid &itemJid, prefs.always)
		{
			oldItems -= itemJid;
			updateItemPrefs(itemJid,ARCHIVE_SAVE_ALWAYS);
		}

		foreach(const Jid &itemJid, oldItems)
		{
			removeItemPrefs(itemJid);
		}

		updateWidget();
		updateColumnsSize();
	}
}

void ArchiveAccountOptionsWidget::onArchivePrefsClosed( const Jid &AStreamJid )
{
	if (AStreamJid == FStreamJid)
		updateWidget();
}

void ArchiveAccountOptionsWidget::onArchiveRequestCompleted(const QString &AId)
{
	if (FSaveRequests.removeOne(AId))
		updateWidget();
}

void ArchiveAccountOptionsWidget::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FSaveRequests.removeOne(AId))
	{
		FLastError = AError;
		updateWidget();
		emit modified();
	}
}
