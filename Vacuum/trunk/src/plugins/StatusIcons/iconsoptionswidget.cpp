#include "iconsoptionswidget.h"

#include <QSet>
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

  FIconFiles = Skin::skinFilesWithDef(SKIN_TYPE_ICONSET,"status","*.jisp");
  for (int i = 0; i < FIconFiles.count(); ++i)
    FIconFiles[i].prepend("status/");

  ui.lwtDefaultIconset->setItemDelegate(new IconsetDelegate(ui.lwtDefaultIconset));
  ui.lwtDefaultIconset->addItems(FIconFiles);
  ui.lwtDefaultIconset->setCurrentRow(FIconFiles.indexOf(FStatusIcons->defaultIconFile()));

  populateRulesTable(ui.twtUserRules,FIconFiles,IStatusIcons::UserRule);
  populateRulesTable(ui.twtDefaultRules,FIconFiles,IStatusIcons::DefaultRule);

  connect(ui.pbtAddUserRule,SIGNAL(clicked()),SLOT(onAddUserRule()));
  connect(ui.pbtDeleteUserRule,SIGNAL(clicked()),SLOT(onDeleteUserRule()));
}

void IconsOptionsWidget::apply()
{
  FStatusIcons->setDefaultIconFile(FIconFiles.value(ui.lwtDefaultIconset->currentRow(),STATUS_ICONSETFILE));
  
  QSet<QString> rules = FStatusIcons->rules(IStatusIcons::UserRule).toSet();
  for (int i =0; i< ui.twtUserRules->rowCount(); ++i)
  {
    QString rule = ui.twtUserRules->item(i,0)->data(Qt::DisplayRole).toString();
    QString iconFile = ui.twtUserRules->item(i,1)->data(Qt::DisplayRole).toString();
    if (!rules.contains(rule) || FStatusIcons->ruleIconFile(rule,IStatusIcons::UserRule)!=iconFile)
      FStatusIcons->insertRule(rule,iconFile,IStatusIcons::UserRule);
    rules -= rule;
  }

  foreach(QString rule,rules)
    FStatusIcons->removeRule(rule,IStatusIcons::UserRule);
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

void IconsOptionsWidget::onAddUserRule()
{
  QTableWidgetItem *rulePattern = new QTableWidgetItem();
  QTableWidgetItem *ruleFile = new QTableWidgetItem(STATUS_ICONSETFILE);
  int row = ui.twtUserRules->rowCount();
  ui.twtUserRules->insertRow(row);
  ui.twtUserRules->setItem(row,0,rulePattern);
  ui.twtUserRules->setItem(row,1,ruleFile);
  ui.twtUserRules->verticalHeader()->setResizeMode(row,QHeaderView::ResizeToContents);
}

void IconsOptionsWidget::onDeleteUserRule()
{
  ui.twtUserRules->removeRow(ui.twtUserRules->currentRow());
}

