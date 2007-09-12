#include "iconsoptionswidget.h"

#include <QSet>
#include <QDir>
#include <QHeaderView>
#include <QComboBox>

//IconsetSelectableDelegate
IconsetSelectableDelegate::IconsetSelectableDelegate(const QStringList &AIconFiles, QObject *AParent)
  : IconsetDelegate(AParent)
{
  FIconFiles = AIconFiles;
}

QWidget *IconsetSelectableDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &/*AOption*/, const QModelIndex &/*AIndex*/) const
{
  if (!FIconFiles.isEmpty())
  {
    QComboBox *comboBox = new QComboBox(AParent);
    comboBox->setItemDelegate(new IconsetDelegate(comboBox));
    comboBox->addItems(FIconFiles);
    return comboBox;
  }
  return NULL;
}

void IconsetSelectableDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
  QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
  if (comboBox)
  {
    QString iconFile = AIndex.data(Qt::DisplayRole).toString();
    comboBox->setCurrentIndex(comboBox->findData(iconFile,Qt::DisplayRole));
  }
}

void IconsetSelectableDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
  QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
  if (comboBox)
  {
    QString iconFile = comboBox->itemData(comboBox->currentIndex(),Qt::DisplayRole).toString();
    AModel->setData(AIndex,iconFile,Qt::DisplayRole);
  }
}

void IconsetSelectableDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &/*AIndex*/) const
{
  AEditor->setGeometry(AOption.rect);
}

//IconsOptionsWidget
IconsOptionsWidget::IconsOptionsWidget(IStatusIcons *AStatusIcons)
{
  ui.setupUi(this);
  FStatusIcons = AStatusIcons;

  QDir statusIconsDir(Skin::pathToSkins()+"/"+Skin::skin()+"/iconset/status","*.jisp",QDir::Name|QDir::IgnoreCase,QDir::Files);
  FIconFiles = statusIconsDir.entryList();
  for (int i = 0; i < FIconFiles.count(); ++i)
    FIconFiles[i].prepend("status/");

  ui.lwtDefaultIconset->setItemDelegate(new IconsetDelegate(ui.lwtDefaultIconset));
  ui.lwtDefaultIconset->addItems(FIconFiles);
  ui.lwtDefaultIconset->setCurrentRow(FIconFiles.indexOf(FStatusIcons->defaultIconFile()));

  populateRulesTable(ui.twtServiceRules,FIconFiles,IStatusIcons::ServiceRule);
  populateRulesTable(ui.twtContactRules,FIconFiles,IStatusIcons::JidRule);
  populateRulesTable(ui.twtCustomRules,FIconFiles,IStatusIcons::CustomRule);

  FAddRuleMapper = new QSignalMapper(this);
  connect(FAddRuleMapper,SIGNAL(mapped(QWidget *)),SLOT(onAddRule(QWidget *)));
  FAddRuleMapper->setMapping(ui.pbtAddService,ui.twtServiceRules);
  connect(ui.pbtAddService,SIGNAL(clicked()),FAddRuleMapper,SLOT(map()));
  FAddRuleMapper->setMapping(ui.pbtAddContact,ui.twtContactRules);
  connect(ui.pbtAddContact,SIGNAL(clicked()),FAddRuleMapper,SLOT(map()));
  FAddRuleMapper->setMapping(ui.pbtAddCustom,ui.twtCustomRules);
  connect(ui.pbtAddCustom,SIGNAL(clicked()),FAddRuleMapper,SLOT(map()));

  FDeleteRuleMapper = new QSignalMapper(this);
  connect(FDeleteRuleMapper,SIGNAL(mapped(QWidget *)),SLOT(onDeleteRule(QWidget *)));
  FDeleteRuleMapper->setMapping(ui.pbtDeleteService,ui.twtServiceRules);
  connect(ui.pbtDeleteService,SIGNAL(clicked()),FDeleteRuleMapper,SLOT(map()));
  FDeleteRuleMapper->setMapping(ui.pbtDeleteContact,ui.twtContactRules);
  connect(ui.pbtDeleteContact,SIGNAL(clicked()),FDeleteRuleMapper,SLOT(map()));
  FDeleteRuleMapper->setMapping(ui.pbtDeleteCustom,ui.twtCustomRules);
  connect(ui.pbtDeleteCustom,SIGNAL(clicked()),FDeleteRuleMapper,SLOT(map()));
}

void IconsOptionsWidget::apply()
{
  FStatusIcons->setDefaultIconFile(FIconFiles.value(ui.lwtDefaultIconset->currentRow(),STATUS_ICONSETFILE));
  applyTableRules(ui.twtServiceRules,IStatusIcons::ServiceRule);
  applyTableRules(ui.twtContactRules,IStatusIcons::JidRule);
  applyTableRules(ui.twtCustomRules,IStatusIcons::CustomRule);
}

void IconsOptionsWidget::populateRulesTable(QTableWidget *ATable, const QStringList &AIconFiles, IStatusIcons::RuleType ARuleType)
{
  int row= 0;
  QStringList rules = FStatusIcons->rules(ARuleType);
  ATable->setItemDelegateForColumn(1,new IconsetSelectableDelegate(AIconFiles,ATable));
  foreach(QString rule, rules)
  {
    QTableWidgetItem *rulePattern = new QTableWidgetItem(rule);
    QTableWidgetItem *ruleFile = new QTableWidgetItem(FStatusIcons->ruleIconFile(rule,ARuleType));
    ATable->insertRow(row);
    ATable->setItem(row,0,rulePattern);
    ATable->setItem(row,1,ruleFile);
    ATable->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
    row++;
  }
  ATable->horizontalHeader()->setResizeMode(0,QHeaderView::Interactive);
  ATable->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
  ATable->verticalHeader()->hide();
}

void IconsOptionsWidget::applyTableRules( QTableWidget *ATable,IStatusIcons::RuleType ARuleType )
{
  QSet<QString> rules = FStatusIcons->rules(ARuleType).toSet();
  for (int i =0; i< ATable->rowCount(); ++i)
  {
    QString rule = ATable->item(i,0)->data(Qt::DisplayRole).toString();
    QString iconFile = ATable->item(i,1)->data(Qt::DisplayRole).toString();
    if (!rules.contains(rule) || FStatusIcons->ruleIconFile(rule,ARuleType)!=iconFile)
      FStatusIcons->insertRule(rule,iconFile,ARuleType);
    rules -= rule;
  }

  foreach (QString rule,rules)
    FStatusIcons->removeRule(rule,ARuleType);
}

void IconsOptionsWidget::onAddRule(QWidget *ATable)
{
  QTableWidget *table = qobject_cast<QTableWidget *>(ATable);
  if (table)
  {
    QTableWidgetItem *rulePattern = new QTableWidgetItem();
    QTableWidgetItem *ruleFile = new QTableWidgetItem(STATUS_ICONSETFILE);
    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row,0,rulePattern);
    table->setItem(row,1,ruleFile);
    table->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
  }
}

void IconsOptionsWidget::onDeleteRule(QWidget *ATable)
{
  QTableWidget *table = qobject_cast<QTableWidget *>(ATable);
  if (table)
    table->removeRow(table->currentRow());
}

