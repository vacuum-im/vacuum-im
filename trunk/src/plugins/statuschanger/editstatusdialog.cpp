#include "editstatusdialog.h"

#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>
#include <QTableWidgetItem>

#define TIR_STATUSID    Qt::UserRole
#define TIR_DELEGATE    Qt::UserRole + 1
#define TIR_VALUE       Qt::UserRole + 2

#define COL_SHOW        0
#define COL_NAME        1
#define COL_MESSAGE     2
#define COL_PRIORITY    3

Delegate::Delegate(IStatusChanger *AStatusChanger, QObject *AParent) : QStyledItemDelegate(AParent)
{
	FStatusChanger = AStatusChanger;
}

QWidget *Delegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
	switch (type)
	{
	case DelegateShow:
	{
		QComboBox *comboBox = new QComboBox(AParent);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::Online),FStatusChanger->nameByShow(IPresence::Online),IPresence::Online);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::Chat),FStatusChanger->nameByShow(IPresence::Chat),IPresence::Chat);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::Away),FStatusChanger->nameByShow(IPresence::Away),IPresence::Away);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::DoNotDisturb),FStatusChanger->nameByShow(IPresence::DoNotDisturb),IPresence::DoNotDisturb);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::ExtendedAway),FStatusChanger->nameByShow(IPresence::ExtendedAway),IPresence::ExtendedAway);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::Invisible),FStatusChanger->nameByShow(IPresence::Invisible),IPresence::Invisible);
		comboBox->addItem(FStatusChanger->iconByShow(IPresence::Offline),FStatusChanger->nameByShow(IPresence::Offline),IPresence::Offline);
		comboBox->setEditable(false);
		return comboBox;
	}
	case DelegatePriority:
	{
		QSpinBox *spinBox = new QSpinBox(AParent);
		spinBox->setMinimum(-128);
		spinBox->setMaximum(128);
		return spinBox;
	}
	default:
		return QStyledItemDelegate::createEditor(AParent,AOption,AIndex);
	}
}

void Delegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
	switch (type)
	{
	case DelegateShow:
	{
		QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
		if (comboBox)
		{
			int show = AIndex.data(TIR_VALUE).toInt();
			comboBox->setCurrentIndex(comboBox->findData(show));
		}
		break;
	}
	case DelegatePriority:
	{
		QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
		if (spinBox)
			spinBox->setValue(AIndex.data(TIR_VALUE).toInt());
	}
	default:
		QStyledItemDelegate::setEditorData(AEditor,AIndex);
	}
}

void Delegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
	bool allowEmptyText = true;
	switch (type)
	{
	case DelegateName:
		allowEmptyText = false;
	case DelegateMessage:
	{
		QLineEdit *lineEdit = qobject_cast<QLineEdit *>(AEditor);
		if (lineEdit && (allowEmptyText || !lineEdit->text().trimmed().isEmpty()))
		{
			AModel->setData(AIndex, lineEdit->text(), Qt::DisplayRole);
			AModel->setData(AIndex, lineEdit->text(), TIR_VALUE);
		}
		break;
	}
	case DelegateShow:
	{
		QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
		if (comboBox)
		{
			int show = comboBox->itemData(comboBox->currentIndex()).toInt();
			AModel->setData(AIndex, FStatusChanger->iconByShow(show), Qt::DecorationRole);
			AModel->setData(AIndex, FStatusChanger->nameByShow(show), Qt::DisplayRole);
			AModel->setData(AIndex, show, TIR_VALUE);
		}
		break;
	}
	case DelegatePriority:
	{
		QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
		if (spinBox)
		{
			AModel->setData(AIndex, spinBox->value(), Qt::DisplayRole);
			AModel->setData(AIndex, spinBox->value(), TIR_VALUE);
		}
	}
	default:
		QStyledItemDelegate::setModelData(AEditor,AModel,AIndex);
	}
}

void Delegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
	switch (type)
	{
	case DelegateShow:
	{
		AEditor->setGeometry(AOption.rect);
		break;
	}
	default:
		QStyledItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
	}
}

//EditStatusDialog
EditStatusDialog::EditStatusDialog(IStatusChanger *AStatusChanger)
{
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SCHANGER_EDIT_STATUSES,0,0,"windowIcon");

	FStatusChanger = AStatusChanger;

	tblStatus->setWordWrap(true);
	tblStatus->setItemDelegate(new Delegate(AStatusChanger,tblStatus));

	int row = 0;
	QList<int> statuses = FStatusChanger->statusItems();
	QMultiMap<QString,int> statusOrdered;
	foreach (int statusId, statuses)
	{
		if (statusId > STATUS_NULL_ID)
		{
			RowStatus *status = new RowStatus;
			status->id = statusId;
			status->name = FStatusChanger->statusItemName(statusId);
			status->show = FStatusChanger->statusItemShow(statusId);
			status->priority = FStatusChanger->statusItemPriority(statusId);
			status->text = FStatusChanger->statusItemText(statusId);
			FStatusItems.insert(statusId,status);

			QString sortString = QString("%1-%2").arg(status->show!=IPresence::Offline ? status->show : 100,5,10,QChar('0')).arg(status->name);
			statusOrdered.insert(sortString,statusId);
		}
	}
	foreach (int statusId, statusOrdered)
	{
		tblStatus->insertRow(tblStatus->rowCount());

		RowStatus *status = FStatusItems.value(statusId);

		QTableWidgetItem *show = new QTableWidgetItem;
		show->setData(TIR_STATUSID,statusId);
		show->setData(Qt::DisplayRole, FStatusChanger->nameByShow(status->show));
		show->setData(Qt::DecorationRole, FStatusChanger->iconByShow(status->show));
		show->setData(TIR_DELEGATE,Delegate::DelegateShow);
		show->setData(TIR_VALUE,status->show);
		tblStatus->setItem(row,COL_SHOW,show);

		QTableWidgetItem *name = new QTableWidgetItem;
		name->setData(Qt::DisplayRole, status->name);
		name->setData(TIR_DELEGATE,Delegate::DelegateName);
		name->setData(TIR_VALUE,status->name);
		tblStatus->setItem(row,COL_NAME,name);

		QTableWidgetItem *message = new QTableWidgetItem;
		message->setData(Qt::DisplayRole, status->text);
		message->setData(TIR_DELEGATE,Delegate::DelegateMessage);
		message->setData(TIR_VALUE,status->text);
		tblStatus->setItem(row,COL_MESSAGE,message);

		QTableWidgetItem *priority = new QTableWidgetItem;
		priority->setTextAlignment(Qt::AlignCenter);
		priority->setData(Qt::DisplayRole, status->priority);
		priority->setData(TIR_DELEGATE,Delegate::DelegatePriority);
		priority->setData(TIR_VALUE,status->priority);
		tblStatus->setItem(row,COL_PRIORITY,priority);

		if (statusId > STATUS_MAX_STANDART_ID)
		{
			show->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		}
		else
		{
			show->setFlags(Qt::ItemIsSelectable);
			name->setFlags(Qt::ItemIsSelectable);
			message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		}

		row++;
	}

	tblStatus->horizontalHeader()->setResizeMode(COL_SHOW,QHeaderView::ResizeToContents);
	tblStatus->horizontalHeader()->setResizeMode(COL_NAME,QHeaderView::ResizeToContents);
	tblStatus->horizontalHeader()->setResizeMode(COL_MESSAGE,QHeaderView::Stretch);
	tblStatus->horizontalHeader()->setResizeMode(COL_PRIORITY,QHeaderView::ResizeToContents);

	connect(pbtAdd,SIGNAL(clicked(bool)),SLOT(onAddbutton(bool)));
	connect(pbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButton(bool)));
	connect(dbtDialogButtons,SIGNAL(accepted()),SLOT(onDialogButtonsBoxAccepted()));
	connect(tblStatus,SIGNAL(itemSelectionChanged()),SLOT(onSelectionChanged()));

	onSelectionChanged();}

EditStatusDialog::~EditStatusDialog()
{
	qDeleteAll(FStatusItems);
}

void EditStatusDialog::onAddbutton(bool)
{
	QString statusName = QInputDialog::getText(this,tr("Enter status name"),tr("Status name:"));
	if (!statusName.isEmpty())
	{
		if (FStatusChanger->statusByName(statusName) == STATUS_NULL_ID)
		{
			int row = tblStatus->rowCount();
			tblStatus->insertRow(row);

			QTableWidgetItem *show = new QTableWidgetItem;
			show->setData(TIR_STATUSID,STATUS_NULL_ID);
			show->setData(Qt::DisplayRole, FStatusChanger->nameByShow(IPresence::Online));
			show->setData(Qt::DecorationRole, FStatusChanger->iconByShow(IPresence::Online));
			show->setData(TIR_DELEGATE,Delegate::DelegateShow);
			show->setData(TIR_VALUE,IPresence::Online);
			tblStatus->setItem(row,COL_SHOW,show);

			QTableWidgetItem *name = new QTableWidgetItem;
			name->setData(Qt::DisplayRole, statusName);
			name->setData(TIR_DELEGATE,Delegate::DelegateName);
			name->setData(TIR_VALUE,statusName);
			tblStatus->setItem(row,COL_NAME,name);

			QTableWidgetItem *message = new QTableWidgetItem;
			message->setData(Qt::DisplayRole, statusName);
			message->setData(TIR_DELEGATE,Delegate::DelegateMessage);
			message->setData(TIR_VALUE,statusName);
			tblStatus->setItem(row,COL_MESSAGE,message);

			QTableWidgetItem *priority = new QTableWidgetItem;
			priority->setTextAlignment(Qt::AlignCenter);
			priority->setData(Qt::DisplayRole, 30);
			priority->setData(TIR_DELEGATE,Delegate::DelegatePriority);
			priority->setData(TIR_VALUE,30);
			tblStatus->setItem(row,COL_PRIORITY,priority);

			tblStatus->editItem(message);
		}
		else
			QMessageBox::warning(this,tr("Wrong status name"),tr("Status with name '<b>%1</b>' already exists").arg(Qt::escape(statusName)));

	}
}

void EditStatusDialog::onDeleteButton(bool)
{
	QList<QTableWidgetItem *> tableItems = tblStatus->selectedItems();
	foreach (QTableWidgetItem *tableItem, tableItems)
	{
		if (tableItem->data(TIR_STATUSID).isValid())
		{
			int statusId = tableItem->data(TIR_STATUSID).toInt();
			if (statusId == STATUS_NULL_ID)
			{
				tblStatus->removeRow(tableItem->row());
			}
			else if (statusId <= STATUS_MAX_STANDART_ID)
			{
				QMessageBox::information(this,tr("Can't delete status"),tr("You can not delete standard statuses."));
			}
			else if (FStatusChanger->activeStatusItems().contains(statusId))
			{
				QMessageBox::information(this,tr("Can't delete status"),tr("You can not delete active statuses."));
			}
			else if (FStatusItems.contains(statusId))
			{
				int button = QMessageBox::question(this,tr("Delete status"),
				                                   tr("You are assured that wish to remove a status '<b>%1</b>'?").arg(Qt::escape(FStatusItems.value(statusId)->name)),
				                                   QMessageBox::Yes | QMessageBox::No);
				if (button == QMessageBox::Yes)
				{
					FDeletedStatuses.append(statusId);
					tblStatus->removeRow(tableItem->row());
				}
			}
			break;
		}
	}
}

void EditStatusDialog::onDialogButtonsBoxAccepted()
{
	foreach (int statusId, FDeletedStatuses)
		FStatusChanger->removeStatusItem(statusId);

	for (int i=0; i<tblStatus->rowCount(); i++)
	{
		int statusId = tblStatus->item(i,0)->data(TIR_STATUSID).toInt();

		int show = tblStatus->item(i,COL_SHOW)->data(TIR_VALUE).toInt();
		QString name = tblStatus->item(i,COL_NAME)->data(TIR_VALUE).toString();
		QString text = tblStatus->item(i,COL_MESSAGE)->data(TIR_VALUE).toString();
		int priority = tblStatus->item(i,COL_PRIORITY)->data(TIR_VALUE).toInt();

		RowStatus *status = FStatusItems.value(statusId,NULL);
		if (!status)
		{
			if (!name.isEmpty() && !text.isEmpty())
				FStatusChanger->addStatusItem(name,show,text,priority);
		}
		else if (status->name != name || status->show != show || status->text != text || status->priority != priority)
		{
			FStatusChanger->updateStatusItem(statusId,name,show,text,priority);
		}
	}
	accept();
}

void EditStatusDialog::onSelectionChanged()
{
	QList<QTableWidgetItem *> tableItems = tblStatus->selectedItems();
	bool allowDelete = true;
	bool selectionEmpty = true;
	foreach (QTableWidgetItem *tableItem, tableItems)
	{
		if (tableItem->data(TIR_STATUSID).isValid())
		{
			int statusId = tableItem->data(TIR_STATUSID).toInt();
			allowDelete = allowDelete && (statusId > STATUS_MAX_STANDART_ID || statusId == STATUS_NULL_ID);
			selectionEmpty = false;
		}
	}
	pbtDelete->setEnabled(allowDelete && !selectionEmpty);
}
