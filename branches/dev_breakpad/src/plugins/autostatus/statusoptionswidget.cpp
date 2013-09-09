#include "statusoptionswidget.h"

#include <QSpinBox>
#include <QComboBox>
#include <QTimeEdit>
#include <QLineEdit>
#include <QMessageBox>
#include <QHeaderView>

#define SDR_VALUE     Qt::UserRole+1

enum Columns {
	COL_ENABLED,
	COL_TIME,
	COL_SHOW,
	COL_TEXT,
	COL_PRIORITY,
	COL__COUNT
};

Delegate::Delegate(IStatusChanger *AStatusChanger, QObject *AParent) : QItemDelegate(AParent)
{
	FStatusChanger = AStatusChanger;
}

QWidget *Delegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_ENABLED:
		{
			return NULL;
		}
	case COL_TIME:
		{
			QTimeEdit *timeEdit = new QTimeEdit(AParent);
			timeEdit->setDisplayFormat("HH:mm:ss");
			return timeEdit;
		}
	case COL_SHOW:
		{
			QComboBox *comboBox = new QComboBox(AParent);
			comboBox->addItem(FStatusChanger->iconByShow(IPresence::Away),FStatusChanger->nameByShow(IPresence::Away),IPresence::Away);
			comboBox->addItem(FStatusChanger->iconByShow(IPresence::DoNotDisturb),FStatusChanger->nameByShow(IPresence::DoNotDisturb),IPresence::DoNotDisturb);
			comboBox->addItem(FStatusChanger->iconByShow(IPresence::ExtendedAway),FStatusChanger->nameByShow(IPresence::ExtendedAway),IPresence::ExtendedAway);
			comboBox->addItem(FStatusChanger->iconByShow(IPresence::Invisible),FStatusChanger->nameByShow(IPresence::Invisible),IPresence::Invisible);
			comboBox->addItem(FStatusChanger->iconByShow(IPresence::Offline),FStatusChanger->nameByShow(IPresence::Offline),IPresence::Offline);
			comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			comboBox->setEditable(false);
			return comboBox;
		}
	case COL_PRIORITY:
		{
			QSpinBox *spinbox = new QSpinBox(AParent);
			spinbox->setMaximum(127);
			spinbox->setMinimum(-128);
			return spinbox;
		}
	default:
		return QItemDelegate::createEditor(AParent,AOption,AIndex);
	}
}

void Delegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_TIME:
		{
			QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(AEditor);
			if (timeEdit)
				timeEdit->setTime(QTime(0,0).addSecs(AIndex.data(SDR_VALUE).toInt()));
		}
		break;
	case COL_SHOW:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
				comboBox->setCurrentIndex(comboBox->findData(AIndex.data(SDR_VALUE).toInt()));
		}
		break;
	case COL_PRIORITY:
		{
			QSpinBox *spinbox = qobject_cast<QSpinBox *>(AEditor);
			if (spinbox)
				spinbox->setValue(AIndex.data(SDR_VALUE).toInt());
		}
		break;
	default:
		QItemDelegate::setEditorData(AEditor,AIndex);
	}
}

void Delegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_TIME:
		{
			QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(AEditor);
			if (timeEdit)
			{
				AModel->setData(AIndex,QTime(0,0).secsTo(timeEdit->time()),SDR_VALUE);
				AModel->setData(AIndex,timeEdit->time().toString(),Qt::DisplayRole);
			}
		}
		break;
	case COL_SHOW:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
			{
				int show = comboBox->itemData(comboBox->currentIndex()).toInt();
				AModel->setData(AIndex, show, SDR_VALUE);
				AModel->setData(AIndex, FStatusChanger->iconByShow(show), Qt::DecorationRole);
				AModel->setData(AIndex, FStatusChanger->nameByShow(show), Qt::DisplayRole);
			}
		}
		break;
	case COL_TEXT:
		{
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(AEditor);
			if (lineEdit)
			{
				AModel->setData(AIndex, lineEdit->text(), SDR_VALUE);
				AModel->setData(AIndex, lineEdit->text(), Qt::DisplayRole);
			}
		}
		break;
	case COL_PRIORITY:
		{
			QSpinBox *spinbox = qobject_cast<QSpinBox *>(AEditor);
			if (spinbox)
			{
				AModel->setData(AIndex, spinbox->value(), SDR_VALUE);
				AModel->setData(AIndex, spinbox->value(), Qt::DisplayRole);
			}
		}
		break;
	default:
		QItemDelegate::setModelData(AEditor,AModel,AIndex);
	}
}

void Delegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case COL_TIME:
		{
			AEditor->setGeometry(AOption.rect);
			AEditor->setMinimumWidth(AEditor->sizeHint().width());
		}
		break;
	case COL_SHOW:
		{
			AEditor->adjustSize();
			QRect rect = AOption.rect;
			rect.setWidth(AEditor->width());
			AEditor->setGeometry(rect);
		}
		break;
	default:
		QItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
	}
}

StatusOptionsWidget::StatusOptionsWidget(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FAutoStatus = AAutoStatus;
	FStatusChanger = AStatusChanger;

	ui.tbwRules->setItemDelegate(new Delegate(FStatusChanger,ui.tbwRules));

	ui.tbwRules->setColumnCount(COL__COUNT);
	ui.tbwRules->setHorizontalHeaderLabels(QStringList() << QString::null << tr("Time") << tr("Status") << tr("Text") << tr("Priority"));

	ui.tbwRules->sortItems(COL_TIME);
	ui.tbwRules->horizontalHeader()->setResizeMode(COL_ENABLED,QHeaderView::ResizeToContents);
	ui.tbwRules->horizontalHeader()->setResizeMode(COL_TIME,QHeaderView::ResizeToContents);
	ui.tbwRules->horizontalHeader()->setResizeMode(COL_SHOW,QHeaderView::ResizeToContents);
	ui.tbwRules->horizontalHeader()->setResizeMode(COL_TEXT,QHeaderView::Stretch);
	ui.tbwRules->horizontalHeader()->setResizeMode(COL_PRIORITY,QHeaderView::ResizeToContents);
	ui.tbwRules->horizontalHeader()->setSortIndicatorShown(false);
	ui.tbwRules->horizontalHeader()->setHighlightSections(false);
	ui.tbwRules->verticalHeader()->hide();

	connect(ui.pbtHelp,SIGNAL(clicked(bool)),SLOT(onHelpButtonClicked(bool)));
	connect(ui.pbtAdd,SIGNAL(clicked(bool)),SLOT(onAddButtonClicked(bool)));
	connect(ui.pbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButtonClicked(bool)));
	connect(ui.tbwRules,SIGNAL(itemChanged(QTableWidgetItem *)),SIGNAL(modified()));

	reset();
}

StatusOptionsWidget::~StatusOptionsWidget()
{

}

void StatusOptionsWidget::apply()
{
	QList<QUuid> oldRules = FAutoStatus->rules();
	for (int i = 0; i<ui.tbwRules->rowCount(); i++)
	{
		IAutoStatusRule rule;
		rule.time = ui.tbwRules->item(i,COL_TIME)->data(SDR_VALUE).toInt();
		rule.show = ui.tbwRules->item(i,COL_SHOW)->data(SDR_VALUE).toInt();
		rule.text = ui.tbwRules->item(i,COL_TEXT)->data(SDR_VALUE).toString();
		rule.priority = ui.tbwRules->item(i,COL_PRIORITY)->data(SDR_VALUE).toInt();

		QUuid ruleId = ui.tbwRules->item(i,COL_ENABLED)->data(SDR_VALUE).toString();
		if (!ruleId.isNull())
		{
			IAutoStatusRule oldRule = FAutoStatus->ruleValue(ruleId);
			if (oldRule.time!=rule.time || oldRule.show!=rule.show || oldRule.text!=rule.text || oldRule.priority!=rule.priority)
				FAutoStatus->updateRule(ruleId,rule);
			oldRules.removeAll(ruleId);
		}
		else
		{
			ruleId = FAutoStatus->insertRule(rule);
			ui.tbwRules->item(i,COL_ENABLED)->setData(SDR_VALUE,ruleId.toString());
		}
		FAutoStatus->setRuleEnabled(ruleId,ui.tbwRules->item(i,COL_ENABLED)->checkState()==Qt::Checked);
	}

	foreach(QUuid ruleId, oldRules)
		FAutoStatus->removeRule(ruleId);

	emit childApply();
}

void StatusOptionsWidget::reset()
{
  ui.pbtDelete->setEnabled(false);
	ui.tbwRules->clearContents();
	ui.tbwRules->setRowCount(0);
	foreach(QUuid ruleId, FAutoStatus->rules()) {
		appendTableRow(ruleId, FAutoStatus->ruleValue(ruleId)); }
	ui.tbwRules->horizontalHeader()->doItemsLayout();
	emit childReset();
}

int StatusOptionsWidget::appendTableRow(const QUuid &ARuleId, const IAutoStatusRule &ARule)
{
	QTableWidgetItem *enabled = new QTableWidgetItem;
	enabled->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
	enabled->setCheckState(FAutoStatus->isRuleEnabled(ARuleId) ? Qt::Checked : Qt::Unchecked);
	enabled->setData(SDR_VALUE,ARuleId.toString());

	QTableWidgetItem *time = new QTableWidgetItem(QTime(0,0).addSecs(ARule.time).toString());
	time->setData(SDR_VALUE,ARule.time);

	QTableWidgetItem *status = new QTableWidgetItem(FStatusChanger->iconByShow(ARule.show),FStatusChanger->nameByShow(ARule.show));
	status->setData(SDR_VALUE,ARule.show);

	QTableWidgetItem *text = new QTableWidgetItem(ARule.text);
	text->setData(SDR_VALUE,ARule.text);

	QTableWidgetItem *priority = new QTableWidgetItem(QString::number(ARule.priority));
	priority->setData(SDR_VALUE,ARule.priority);

	int curRow = ui.tbwRules->rowCount();
	ui.tbwRules->setRowCount(curRow+1);
	ui.tbwRules->setItem(curRow,COL_ENABLED,enabled);
	ui.tbwRules->setItem(enabled->row(),COL_TIME,time);
	ui.tbwRules->setItem(enabled->row(),COL_SHOW,status);
	ui.tbwRules->setItem(enabled->row(),COL_TEXT,text);
	ui.tbwRules->setItem(enabled->row(),COL_PRIORITY,priority);

  ui.pbtDelete->setEnabled(ui.tbwRules->rowCount() > 1);

	return enabled->row();
}

void StatusOptionsWidget::onHelpButtonClicked(bool)
{
  QMessageBox::information(this,tr("Auto Status"),
    tr("You can insert date and time into auto status text:") + "\n" + \
    tr("   %(<format>) - current date and time") + "\n" + \
    tr("   $(<format>) - date and time you are idle form") + "\n" + \
    tr("   #(<format>) - time you are idle for") + "\n\n" + \
    tr("Date Format:") + "\n" + \
    tr("   d - the day as number without a leading zero (1 to 31)") + "\n" + \
    tr("   dd - the day as number with a leading zero (01 to 31)") + "\n" + \
    tr("   ddd - the abbreviated localized day name (e.g. 'Mon' to 'Sun')") + "\n" + \
    tr("   dddd - the long localized day name (e.g. 'Monday' to 'Sunday')") + "\n" + \
    tr("   M - the month as number without a leading zero (1-12)") + "\n" + \
    tr("   MM - the month as number with a leading zero (01-12)") + "\n" + \
    tr("   MMM - the abbreviated localized month name (e.g. 'Jan' to 'Dec')") + "\n" + \
    tr("   MMMM - the long localized month name (e.g. 'January' to 'December')") + "\n" + \
    tr("   yy - the year as two digit number (00-99)") + "\n" + \
    tr("   yyyy - the year as four digit number") + "\n\n" + \
    tr("Time Format:") + "\n" + \
    tr("   h - the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)") + "\n" + \
    tr("   hh - the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)") + "\n" + \
    tr("   H - the hour without a leading zero (0 to 23, even with AM/PM display)") + "\n" + \
    tr("   HH - the hour with a leading zero (00 to 23, even with AM/PM display)") + "\n" + \
    tr("   m - the minute without a leading zero (0 to 59)") + "\n" + \
    tr("   mm - the minute with a leading zero (00 to 59)") + "\n" + \
    tr("   s - the second without a leading zero (0 to 59)") + "\n" + \
    tr("   ss - the second with a leading zero (00 to 59)") + "\n" + \
    tr("   z - the milliseconds without leading zeroes (0 to 999)") + "\n" + \
    tr("   zzz - the milliseconds with leading zeroes (000 to 999)") + "\n" + \
    tr("   AP or A - interpret as an AM/PM time. AP must be either 'AM' or 'PM'") + "\n" + \
    tr("   ap or a - interpret as an AM/PM time. ap must be either 'am' or 'pm'") + "\n\n" + \
    tr("Example:") + "\n" + \
    tr("   Status is set to 'away' at %(hh:mm:ss), because of idle from $(hh:mm:ss) for #(mm) minutes and #(ss) seconds")
    );
}

void StatusOptionsWidget::onAddButtonClicked(bool)
{
	IAutoStatusRule rule;
	if (ui.tbwRules->rowCount()>0)
		rule.time = ui.tbwRules->item(ui.tbwRules->rowCount()-1,COL_TIME)->data(SDR_VALUE).toInt() + 5*60;
	else
		rule.time = 10*60;
	rule.show = IPresence::Away;
	rule.text = tr("Auto status");
	rule.priority = 20;
	ui.tbwRules->setCurrentCell(appendTableRow(QUuid(),rule),COL_ENABLED);
	ui.tbwRules->horizontalHeader()->doItemsLayout();
	emit modified();
}

void StatusOptionsWidget::onDeleteButtonClicked(bool)
{
	QTableWidgetItem *item = ui.tbwRules->currentItem();
	if (item)
	{
		ui.tbwRules->removeRow(item->row());
    ui.pbtDelete->setEnabled(ui.tbwRules->rowCount() > 1);
		emit modified();
	}
}
