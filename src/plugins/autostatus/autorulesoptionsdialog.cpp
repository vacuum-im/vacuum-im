#include "autorulesoptionsdialog.h"

#include <QSpinBox>
#include <QTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <utils/logger.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>

enum RulesTableColumns {
	RTC_ENABLED,
	RTC_TIME,
	RTC_SHOW,
	RTC_TEXT,
	RTC_PRIORITY,
	RTC__COUNT
};

enum RulesTableRoles {
	SDR_VALUE = Qt::UserRole+1
};

AutoRuleDelegate::AutoRuleDelegate(IStatusChanger *AStatusChanger, QObject *AParent) : QStyledItemDelegate(AParent)
{
	FStatusChanger = AStatusChanger;
}

QWidget *AutoRuleDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case RTC_ENABLED:
		{
			return NULL;
		}
	case RTC_TIME:
		{
			QTimeEdit *timeEdit = new QTimeEdit(AParent);
			timeEdit->setDisplayFormat("HH:mm:ss");
			return timeEdit;
		}
	case RTC_SHOW:
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
	case RTC_PRIORITY:
		{
			QSpinBox *spinbox = new QSpinBox(AParent);
			spinbox->setMaximum(127);
			spinbox->setMinimum(-128);
			return spinbox;
		}
	default:
		return QStyledItemDelegate::createEditor(AParent,AOption,AIndex);
	}
}

void AutoRuleDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case RTC_TIME:
		{
			QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(AEditor);
			if (timeEdit)
				timeEdit->setTime(QTime(0,0).addSecs(AIndex.data(SDR_VALUE).toInt()));
		}
		break;
	case RTC_SHOW:
		{
			QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
			if (comboBox)
				comboBox->setCurrentIndex(comboBox->findData(AIndex.data(SDR_VALUE).toInt()));
		}
		break;
	case RTC_PRIORITY:
		{
			QSpinBox *spinbox = qobject_cast<QSpinBox *>(AEditor);
			if (spinbox)
				spinbox->setValue(AIndex.data(SDR_VALUE).toInt());
		}
		break;
	default:
		QStyledItemDelegate::setEditorData(AEditor,AIndex);
	}
}

void AutoRuleDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case RTC_TIME:
		{
			QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(AEditor);
			if (timeEdit)
			{
				AModel->setData(AIndex,QTime(0,0).secsTo(timeEdit->time()),SDR_VALUE);
				AModel->setData(AIndex,timeEdit->time().toString(),Qt::DisplayRole);
			}
		}
		break;
	case RTC_SHOW:
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
	case RTC_TEXT:
		{
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(AEditor);
			if (lineEdit)
			{
				AModel->setData(AIndex, lineEdit->text(), SDR_VALUE);
				AModel->setData(AIndex, lineEdit->text(), Qt::DisplayRole);
			}
		}
		break;
	case RTC_PRIORITY:
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
		QStyledItemDelegate::setModelData(AEditor,AModel,AIndex);
	}
}

void AutoRuleDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	switch (AIndex.column())
	{
	case RTC_TIME:
		{
			AEditor->setGeometry(AOption.rect);
			AEditor->setMinimumWidth(AEditor->sizeHint().width());
		}
		break;
	case RTC_SHOW:
		{
			AEditor->adjustSize();
			QRect rect = AOption.rect;
			rect.setWidth(AEditor->width());
			AEditor->setGeometry(rect);
		}
		break;
	default:
		QStyledItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
	}
}


AutoRulesOptionsDialog::AutoRulesOptionsDialog(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Auto Status Rules"));

	FAutoStatus = AAutoStatus;
	FStatusChanger = AStatusChanger;

	tbwRules = new QTableWidget(this);
	tbwRules->verticalHeader()->hide();
	tbwRules->horizontalHeader()->setSortIndicatorShown(false);
	tbwRules->setItemDelegate(new AutoRuleDelegate(FStatusChanger,tbwRules));

	tbwRules->setColumnCount(RTC__COUNT);
	tbwRules->setSelectionMode(QTableWidget::SingleSelection);
	tbwRules->setSelectionBehavior(QTableWidget::SelectRows);
	connect(tbwRules,SIGNAL(itemSelectionChanged()),SLOT(onRuledItemSelectionChanged()));

	tbwRules->setHorizontalHeaderLabels(QStringList() << QString::null << tr("Time") << tr("Status") << tr("Text") << tr("Priority"));
	tbwRules->horizontalHeader()->setSectionResizeMode(RTC_ENABLED,QHeaderView::ResizeToContents);
	tbwRules->horizontalHeader()->setSectionResizeMode(RTC_TIME,QHeaderView::ResizeToContents);
	tbwRules->horizontalHeader()->setSectionResizeMode(RTC_SHOW,QHeaderView::ResizeToContents);
	tbwRules->horizontalHeader()->setSectionResizeMode(RTC_TEXT,QHeaderView::Stretch);
	tbwRules->horizontalHeader()->setSectionResizeMode(RTC_PRIORITY,QHeaderView::ResizeToContents);
	tbwRules->horizontalHeader()->setHighlightSections(false);

	dbbButtonBox = new QDialogButtonBox(this);
	dbbButtonBox->addButton(QDialogButtonBox::Ok);
	dbbButtonBox->addButton(QDialogButtonBox::Cancel);
	pbtAdd = dbbButtonBox->addButton(tr("Add"),QDialogButtonBox::ActionRole);
	pbtDelete = dbbButtonBox->addButton(tr("Delete"),QDialogButtonBox::ActionRole);
	connect(dbbButtonBox,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonBoxClicked(QAbstractButton *)));

	QHBoxLayout *hblButtons = new QHBoxLayout;
	hblButtons->addWidget(pbtAdd);
	hblButtons->addWidget(pbtDelete);
	hblButtons->addStretch();
	hblButtons->addWidget(dbbButtonBox);

	QVBoxLayout *vblLayout = new QVBoxLayout(this);
	vblLayout->setMargin(5);
	vblLayout->addWidget(tbwRules);
	vblLayout->addLayout(hblButtons);

	tbwRules->setRowCount(0);
	tbwRules->clearContents();
	foreach(const QUuid &ruleId, FAutoStatus->rules())
		appendTableRow(ruleId, FAutoStatus->ruleValue(ruleId));
	tbwRules->sortItems(RTC_TIME);

	if (!restoreGeometry(Options::fileValue("statuses.autostatus.rules-dialog.geometry").toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(600,400),this));

	onRuledItemSelectionChanged();
}

AutoRulesOptionsDialog::~AutoRulesOptionsDialog()
{
	Options::setFileValue(saveGeometry(),"statuses.autostatus.rules-dialog.geometry");
}

int AutoRulesOptionsDialog::appendTableRow(const QUuid &ARuleId, const IAutoStatusRule &ARule)
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

	int curRow = tbwRules->rowCount();
	tbwRules->setRowCount(curRow+1);
	tbwRules->setItem(curRow,RTC_ENABLED,enabled);
	tbwRules->setItem(enabled->row(),RTC_TIME,time);
	tbwRules->setItem(enabled->row(),RTC_SHOW,status);
	tbwRules->setItem(enabled->row(),RTC_TEXT,text);
	tbwRules->setItem(enabled->row(),RTC_PRIORITY,priority);

	return enabled->row();
}

void AutoRulesOptionsDialog::onRuledItemSelectionChanged()
{
	pbtDelete->setEnabled(!tbwRules->selectedItems().isEmpty());
}

void AutoRulesOptionsDialog::onDialogButtonBoxClicked(QAbstractButton *AButton)
{
	if (AButton == pbtAdd)
	{
		IAutoStatusRule rule;
		if (tbwRules->rowCount() > 0)
			rule.time = tbwRules->item(tbwRules->rowCount()-1,RTC_TIME)->data(SDR_VALUE).toInt() + 5*60;
		else
			rule.time = 10*60;
		rule.priority = 20;
		rule.show = IPresence::Away;
		rule.text = tr("Auto status");
		tbwRules->setCurrentCell(appendTableRow(QUuid(),rule),RTC_ENABLED);
	}
	else if (AButton == pbtDelete)
	{
		QTableWidgetItem *item = tbwRules->currentItem();
		if (item)
			tbwRules->removeRow(item->row());
	}
	else if (dbbButtonBox->buttonRole(AButton) == QDialogButtonBox::AcceptRole)
	{
		QList<QUuid> oldRules = FAutoStatus->rules();
		for (int row = 0; row<tbwRules->rowCount(); row++)
		{
			IAutoStatusRule rule;
			rule.time = tbwRules->item(row,RTC_TIME)->data(SDR_VALUE).toInt();
			rule.show = tbwRules->item(row,RTC_SHOW)->data(SDR_VALUE).toInt();
			rule.text = tbwRules->item(row,RTC_TEXT)->data(SDR_VALUE).toString();
			rule.priority = tbwRules->item(row,RTC_PRIORITY)->data(SDR_VALUE).toInt();

			QUuid ruleId = tbwRules->item(row,RTC_ENABLED)->data(SDR_VALUE).toString();
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
				tbwRules->item(row,RTC_ENABLED)->setData(SDR_VALUE,ruleId.toString());
			}
			FAutoStatus->setRuleEnabled(ruleId,tbwRules->item(row,RTC_ENABLED)->checkState()==Qt::Checked);
		}

		foreach(const QUuid &ruleId, oldRules)
			FAutoStatus->removeRule(ruleId);

		accept();
	}
	else if (dbbButtonBox->buttonRole(AButton) == QDialogButtonBox::RejectRole)
	{
		reject();
	}
}
