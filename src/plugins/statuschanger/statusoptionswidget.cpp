#include "statusoptionswidget.h"

#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>

enum StatusTableColumns {
	STC_STATUS,
	STC_NAME,
	STC_MESSAGE,
	STC_PRIORITY,
	STC__COUNT
};

enum StatusTableRoles {
	STR_STATUSID = Qt::UserRole,
	STR_COLUMN,
	STR_VALUE
};


StatusDelegate::StatusDelegate(IStatusChanger *AStatusChanger, QObject *AParent) : QStyledItemDelegate(AParent)
{
	FStatusChanger = AStatusChanger;
}

QWidget *StatusDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.data(STR_COLUMN).toInt())
	{
	case STC_STATUS:
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
	case STC_PRIORITY:
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

void StatusDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	switch (AIndex.data(STR_COLUMN).toInt())
	{
	case STC_STATUS:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
			{
				int show = AIndex.data(STR_VALUE).toInt();
				comboBox->setCurrentIndex(comboBox->findData(show));
			}
			break;
		}
	case STC_PRIORITY:
		{
			QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
			if (spinBox)
				spinBox->setValue(AIndex.data(STR_VALUE).toInt());
			break;
		}
	default:
		QStyledItemDelegate::setEditorData(AEditor,AIndex);
	}
}

void StatusDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	bool allowEmptyText = true;
	switch (AIndex.data(STR_COLUMN).toInt())
	{
	case STC_NAME:
		allowEmptyText = false;
	case STC_MESSAGE:
		{
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(AEditor);
			if (lineEdit && (allowEmptyText || !lineEdit->text().trimmed().isEmpty()))
			{
				QString data = lineEdit->text();
				AModel->setData(AIndex,data,Qt::DisplayRole);
				AModel->setData(AIndex,data,STR_VALUE);
			}
			break;
		}
	case STC_STATUS:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
			{
				int data = comboBox->itemData(comboBox->currentIndex()).toInt();
				AModel->setData(AIndex,FStatusChanger->iconByShow(data),Qt::DecorationRole);
				AModel->setData(AIndex,FStatusChanger->nameByShow(data),Qt::DisplayRole);
				AModel->setData(AIndex,data,STR_VALUE);
			}
			break;
		}
	case STC_PRIORITY:
		{
			QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
			if (spinBox)
			{
				int data = spinBox->value();
				AModel->setData(AIndex,data,Qt::DisplayRole);
				AModel->setData(AIndex,data,STR_VALUE);
			}
			break;
		}
	default:
		QStyledItemDelegate::setModelData(AEditor,AModel,AIndex);
	}
}

void StatusDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.data(STR_COLUMN).toInt())
	{
	case STC_STATUS:
	case STC_PRIORITY:
		{
			QRect rect = AOption.rect;
			rect.setWidth(qMax(rect.width(),AEditor->sizeHint().width()));
			AEditor->setGeometry(rect);
			break;
		}
	default:
		QStyledItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
	}
}


StatusOptionsWidget::StatusOptionsWidget(IStatusChanger *AStatusChanger, QWidget *AParent) : QWidget(AParent)
{
	FStatusChanger = AStatusChanger;

	pbtAdd = new QPushButton(this);
	pbtAdd->setText(tr("Add"));
	connect(pbtAdd,SIGNAL(clicked(bool)),SLOT(onAddButtonClicked()));

	pbtDelete = new QPushButton(this);
	pbtDelete->setText(tr("Delete"));
	connect(pbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButtonClicked()));

	tbwStatus = new QTableWidget(this);
	tbwStatus->setWordWrap(true);
	tbwStatus->setColumnCount(STC__COUNT);
	tbwStatus->verticalHeader()->setVisible(false);
	tbwStatus->horizontalHeader()->setHighlightSections(false);
	tbwStatus->setSelectionMode(QTableWidget::SingleSelection);
	tbwStatus->setSelectionBehavior(QTableWidget::SelectRows);
	tbwStatus->setItemDelegate(new StatusDelegate(AStatusChanger, tbwStatus));
	connect(tbwStatus,SIGNAL(itemChanged(QTableWidgetItem *)),SIGNAL(modified()));
	connect(tbwStatus,SIGNAL(itemSelectionChanged()),SLOT(onStatusItemSelectionChanged()));

	tbwStatus->setHorizontalHeaderLabels(QStringList() << tr("Status") << tr("Name") << tr("Message") << tr("Priority"));
	tbwStatus->horizontalHeader()->setResizeMode(STC_STATUS,QHeaderView::ResizeToContents);
	tbwStatus->horizontalHeader()->setResizeMode(STC_NAME,QHeaderView::ResizeToContents);
	tbwStatus->horizontalHeader()->setResizeMode(STC_MESSAGE,QHeaderView::Stretch);
	tbwStatus->horizontalHeader()->setResizeMode(STC_PRIORITY,QHeaderView::ResizeToContents);

	QHBoxLayout *hltlayout = new QHBoxLayout;
	hltlayout->setMargin(0);
	hltlayout->addStretch();
	hltlayout->addWidget(pbtAdd);
	hltlayout->addWidget(pbtDelete);

	QVBoxLayout *vltLayout = new QVBoxLayout(this);
	vltLayout->setMargin(0);
	vltLayout->addWidget(tbwStatus);
	vltLayout->addLayout(hltlayout);

	reset();
}

void StatusOptionsWidget::apply()
{
	foreach (int statusId, FDeletedStatuses)
		FStatusChanger->removeStatusItem(statusId);
	FDeletedStatuses.clear();

	for (int row=0; row<tbwStatus->rowCount(); row++)
	{
		int statusId = tbwStatus->item(row,STC_STATUS)->data(STR_STATUSID).toInt();

		int show = tbwStatus->item(row,STC_STATUS)->data(STR_VALUE).toInt();
		QString name = tbwStatus->item(row,STC_NAME)->data(STR_VALUE).toString();
		QString text = tbwStatus->item(row,STC_MESSAGE)->data(STR_VALUE).toString();
		int priority = tbwStatus->item(row,STC_PRIORITY)->data(STR_VALUE).toInt();

		RowData status = FStatusItems.value(statusId);
		if (statusId <= STATUS_NULL_ID)
		{
			int name_i = 1;
			while (name.isEmpty() || FStatusChanger->statusByName(name)!=STATUS_NULL_ID)
			{
				name += QString("_%1").arg(name_i++);
				tbwStatus->item(row,STC_NAME)->setData(STR_VALUE,name);
				tbwStatus->item(row,STC_NAME)->setData(Qt::DisplayRole,name);
			}

			status.show = show;
			status.name = name;
			status.text = text;
			status.priority = priority;

			statusId = FStatusChanger->addStatusItem(name,show,text,priority);
			tbwStatus->item(row,STC_STATUS)->setData(STR_STATUSID, statusId);
			FStatusItems.insert(statusId,status);
		}
		else if (status.name!=name || status.show!=show || status.text!=text || status.priority!=priority)
		{
			FStatusChanger->updateStatusItem(statusId,name,show,text,priority);
		}
	}

	emit childApply();
}

void StatusOptionsWidget::reset()
{
	tbwStatus->clearContents();
	FDeletedStatuses.clear();
	FStatusItems.clear();

	int row = 0;
	foreach(int statusId, FStatusChanger->statusItems())
	{
		if (statusId > STATUS_NULL_ID)
		{
			RowData status;
			status.show = FStatusChanger->statusItemShow(statusId);
			status.name = FStatusChanger->statusItemName(statusId);
			status.text = FStatusChanger->statusItemText(statusId);
			status.priority = FStatusChanger->statusItemPriority(statusId);
			FStatusItems.insert(statusId,status);

			tbwStatus->setRowCount(row+1);

			QTableWidgetItem *name = new QTableWidgetItem;
			name->setData(Qt::DisplayRole, status.name);
			name->setData(STR_COLUMN,STC_NAME);
			name->setData(STR_VALUE,status.name);
			tbwStatus->setItem(row,STC_NAME,name);

			QTableWidgetItem *show = new QTableWidgetItem;
			show->setData(STR_STATUSID,statusId);
			show->setData(Qt::DisplayRole, FStatusChanger->nameByShow(status.show));
			show->setData(Qt::DecorationRole, FStatusChanger->iconByShow(status.show));
			show->setData(STR_COLUMN,STC_STATUS);
			show->setData(STR_VALUE,status.show);
			tbwStatus->setItem(row,STC_STATUS,show);

			QTableWidgetItem *message = new QTableWidgetItem;
			message->setData(Qt::DisplayRole, status.text);
			message->setData(STR_COLUMN,STC_MESSAGE);
			message->setData(STR_VALUE,status.text);
			tbwStatus->setItem(row,STC_MESSAGE,message);

			QTableWidgetItem *priority = new QTableWidgetItem;
			priority->setTextAlignment(Qt::AlignCenter);
			priority->setData(Qt::DisplayRole, status.priority);
			priority->setData(STR_COLUMN,STC_PRIORITY);
			priority->setData(STR_VALUE,status.priority);
			tbwStatus->setItem(row,STC_PRIORITY,priority);

			if (statusId > STATUS_MAX_STANDART_ID)
			{
				show->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
				name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
				message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
				priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			}
			else
			{
				show->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
				priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			}

			tbwStatus->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
			row++;
		}
	}

	emit childReset();
}

void StatusOptionsWidget::onAddButtonClicked()
{
	int row = tbwStatus->rowCount();
	tbwStatus->setRowCount(row+1);

	QTableWidgetItem *name = new QTableWidgetItem;
	name->setData(Qt::DisplayRole,tr("Name"));
	name->setData(STR_COLUMN,STC_NAME);
	name->setData(STR_VALUE,name->data(Qt::DisplayRole));
	tbwStatus->setItem(row,STC_NAME,name);

	QTableWidgetItem *show = new QTableWidgetItem;
	show->setData(STR_STATUSID,STATUS_NULL_ID);
	show->setData(Qt::DisplayRole,FStatusChanger->nameByShow(IPresence::Online));
	show->setData(Qt::DecorationRole,FStatusChanger->iconByShow(IPresence::Online));
	show->setData(STR_COLUMN,STC_STATUS);
	show->setData(STR_VALUE,IPresence::Online);
	tbwStatus->setItem(row,STC_STATUS,show);

	QTableWidgetItem *message = new QTableWidgetItem;
	message->setData(Qt::DisplayRole,tr("Message"));
	message->setData(STR_COLUMN,STC_MESSAGE);
	message->setData(STR_VALUE,message->data(Qt::DisplayRole));
	tbwStatus->setItem(row,STC_MESSAGE,message);

	QTableWidgetItem *priority = new QTableWidgetItem;
	priority->setTextAlignment(Qt::AlignCenter);
	priority->setData(Qt::DisplayRole,30);
	priority->setData(STR_COLUMN,STC_PRIORITY);
	priority->setData(STR_VALUE,30);
	tbwStatus->setItem(row,STC_PRIORITY,priority);

	tbwStatus->editItem(name);

	emit modified();
}

void StatusOptionsWidget::onDeleteButtonClicked()
{
	foreach (QTableWidgetItem *tableItem, tbwStatus->selectedItems())
	{
		if (tableItem->data(STR_STATUSID).isValid())
		{
			int statusId = tableItem->data(STR_STATUSID).toInt();
			if (statusId == STATUS_NULL_ID)
			{
				tbwStatus->removeRow(tableItem->row());
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
				FDeletedStatuses.append(statusId);
				tbwStatus->removeRow(tableItem->row());
			}

			emit modified();
			break;
		}
	}
}

void StatusOptionsWidget::onStatusItemSelectionChanged()
{
	bool allowDelete = true;
	bool selectionEmpty = true;
	foreach (QTableWidgetItem *tableItem, tbwStatus->selectedItems())
	{
		if (tableItem->data(STR_STATUSID).isValid())
		{
			int statusId = tableItem->data(STR_STATUSID).toInt();
			allowDelete = allowDelete && (statusId > STATUS_MAX_STANDART_ID || statusId == STATUS_NULL_ID);
			selectionEmpty = false;
		}
	}
	pbtDelete->setEnabled(allowDelete && !selectionEmpty);
}
