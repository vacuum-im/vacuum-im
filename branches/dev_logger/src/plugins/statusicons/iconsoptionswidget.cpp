#include "iconsoptionswidget.h"

#include <QSet>
#include <QComboBox>
#include <QHeaderView>

//IconsetSelectableDelegate
IconsetSelectableDelegate::IconsetSelectableDelegate(const QString &AStorage, const QStringList &ASubStorages, QObject *AParent) : IconsetDelegate(AParent)
{
	FStorage = AStorage;
	FSubStorages = ASubStorages;
}

QWidget *IconsetSelectableDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(AOption);
	Q_UNUSED(AIndex);
	if (!FSubStorages.isEmpty())
	{
		QComboBox *comboBox = new QComboBox(AParent);
		comboBox->setItemDelegate(new IconsetDelegate(comboBox));
		for (int i=0; i<FSubStorages.count(); i++)
		{
			comboBox->addItem(FStorage+"/"+FSubStorages.at(i));
			comboBox->setItemData(i,FStorage,IDR_STORAGE_NAME);
			comboBox->setItemData(i,FSubStorages.at(i),IDR_STORAGE_SUBDIR);
			comboBox->setItemData(i,1,IDR_ICON_ROWS);
		}
		return comboBox;
	}
	return NULL;
}

void IconsetSelectableDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
	if (comboBox)
	{
		QString substorage = AIndex.data(IDR_STORAGE_SUBDIR).toString();
		comboBox->setCurrentIndex(comboBox->findData(substorage,IDR_STORAGE_SUBDIR));
	}
}

void IconsetSelectableDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
	if (comboBox)
	{
		QString subStorage = comboBox->itemData(comboBox->currentIndex(),IDR_STORAGE_SUBDIR).toString();
		AModel->setData(AIndex,subStorage,IDR_STORAGE_SUBDIR);
	}
}

void IconsetSelectableDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(AIndex);
	AEditor->setGeometry(AOption.rect);
}

//IconsOptionsWidget
IconsOptionsWidget::IconsOptionsWidget(IStatusIcons *AStatusIcons, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FStatusIcons = AStatusIcons;
	FSubStorages.append(FILE_STORAGE_SHARED_DIR);
	FSubStorages += FileStorage::availSubStorages(RSR_STORAGE_STATUSICONS);

	ui.lwtDefaultIconset->setItemDelegate(new IconsetDelegate(ui.lwtDefaultIconset));
	for (int i=0; i<FSubStorages.count(); i++)
	{
		QListWidgetItem *item = new QListWidgetItem(RSR_STORAGE_STATUSICONS"/"+FSubStorages.at(i),ui.lwtDefaultIconset);
		item->setData(IDR_STORAGE_NAME,RSR_STORAGE_STATUSICONS);
		item->setData(IDR_STORAGE_SUBDIR,FSubStorages.at(i));
		item->setData(IDR_ICON_ROWS,1);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		ui.lwtDefaultIconset->addItem(item);
	}

	connect(ui.pbtAddUserRule,SIGNAL(clicked()),SLOT(onAddUserRule()));
	connect(ui.pbtDeleteUserRule,SIGNAL(clicked()),SLOT(onDeleteUserRule()));
	connect(ui.lwtDefaultIconset,SIGNAL(itemChanged(QListWidgetItem *)),SLOT(onDefaultListItemChanged(QListWidgetItem *)));

	connect(ui.twtDefaultRules,SIGNAL(itemChanged(QTableWidgetItem *)),SIGNAL(modified()));
	connect(ui.twtUserRules,SIGNAL(itemChanged(QTableWidgetItem *)),SIGNAL(modified()));

	reset();
}

void IconsOptionsWidget::apply()
{
	for (int row=0; row<ui.lwtDefaultIconset->count(); row++)
		if (ui.lwtDefaultIconset->item(row)->checkState() == Qt::Checked)
		{
			Options::node(OPV_STATUSICONS_DEFAULT).setValue(ui.lwtDefaultIconset->item(row)->data(IDR_STORAGE_SUBDIR));
			break;
		}

	QSet<QString> rules = FStatusIcons->rules(IStatusIcons::UserRule).toSet();
	for (int row =0; row<ui.twtUserRules->rowCount(); row++)
	{
		QString rule = ui.twtUserRules->item(row,0)->data(Qt::DisplayRole).toString();
		QString substorage = ui.twtUserRules->item(row,1)->data(IDR_STORAGE_SUBDIR).toString();
		if (!rules.contains(rule) || FStatusIcons->ruleIconset(rule,IStatusIcons::UserRule)!=substorage)
			FStatusIcons->insertRule(rule,substorage,IStatusIcons::UserRule);
		rules -= rule;
	}

	foreach(const QString &rule,rules)
		FStatusIcons->removeRule(rule,IStatusIcons::UserRule);

	emit childApply();
}

void IconsOptionsWidget::reset()
{
	QString defIconset = Options::node(OPV_STATUSICONS_DEFAULT).value().toString();
	for (int row = 0; row<ui.lwtDefaultIconset->count(); row++)
	{
		QListWidgetItem *item = ui.lwtDefaultIconset->item(row);
		item->setCheckState(item->data(IDR_STORAGE_SUBDIR).toString()==defIconset ? Qt::Checked : Qt::Unchecked);
	}

	ui.twtDefaultRules->clearContents();
	ui.twtDefaultRules->setRowCount(0);
	populateRulesTable(ui.twtDefaultRules,IStatusIcons::DefaultRule);

	ui.twtUserRules->clearContents();
	ui.twtUserRules->setRowCount(0);
	populateRulesTable(ui.twtUserRules,IStatusIcons::UserRule);

	emit childReset();
}

void IconsOptionsWidget::populateRulesTable(QTableWidget *ATable, IStatusIcons::RuleType ARuleType)
{
	int row= 0;
	QStringList rules = FStatusIcons->rules(ARuleType);
	ATable->setItemDelegateForColumn(1,new IconsetSelectableDelegate(RSR_STORAGE_STATUSICONS,FSubStorages,ATable));
	foreach(const QString &rule, rules)
	{
		QString substorage = FStatusIcons->ruleIconset(rule,ARuleType);
		QTableWidgetItem *rulePattern = new QTableWidgetItem(rule);
		QTableWidgetItem *ruleStorage = new QTableWidgetItem();
		ruleStorage->setData(IDR_STORAGE_NAME,RSR_STORAGE_STATUSICONS);
		ruleStorage->setData(IDR_STORAGE_SUBDIR,substorage);
		ruleStorage->setData(IDR_ICON_ROWS,1);
		ATable->insertRow(row);
		ATable->setItem(row,0,rulePattern);
		ATable->setItem(row,1,ruleStorage);
		ATable->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
		row++;
	}
	ATable->horizontalHeader()->setResizeMode(0,QHeaderView::Interactive);
	ATable->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
	ATable->verticalHeader()->hide();
}

void IconsOptionsWidget::onAddUserRule()
{
	QTableWidgetItem *rulePattern = new QTableWidgetItem();
	QTableWidgetItem *ruleStorage = new QTableWidgetItem();
	ruleStorage->setData(IDR_STORAGE_NAME,RSR_STORAGE_STATUSICONS);
	ruleStorage->setData(IDR_STORAGE_SUBDIR,FILE_STORAGE_SHARED_DIR);
	ruleStorage->setData(IDR_ICON_ROWS,1);
	int row = ui.twtUserRules->rowCount();
	ui.twtUserRules->insertRow(row);
	ui.twtUserRules->setItem(row,0,rulePattern);
	ui.twtUserRules->setItem(row,1,ruleStorage);
	ui.twtUserRules->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
	emit modified();
}

void IconsOptionsWidget::onDeleteUserRule()
{
	ui.twtUserRules->removeRow(ui.twtUserRules->currentRow());
	emit modified();
}

void IconsOptionsWidget::onDefaultListItemChanged(QListWidgetItem *AItem)
{
	if (AItem->checkState() == Qt::Checked)
	{
		for (int i=0; i<ui.lwtDefaultIconset->count();i++)
			if (ui.lwtDefaultIconset->item(i)!=AItem)
				ui.lwtDefaultIconset->item(i)->setCheckState(Qt::Unchecked);
		emit modified();
	}
}
