#include "optionsdialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollBar>
#include <QPushButton>
#include <QHeaderView>
#include <QTextDocument>

OptionsDialog::OptionsDialog(QWidget *AParent) : QDialog(AParent)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Options"));

  lblInfo = new QLabel("<b>Node name</b><br>Node description");
  lblInfo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  lblInfo->setFrameStyle(QFrame::StyledPanel);

  scaScroll = new QScrollArea;
  scaScroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  scaScroll->setWidgetResizable(true);

  stwOptions = new QStackedWidget;
  scaScroll->setWidget(stwOptions);
  
  QVBoxLayout *vblRight = new QVBoxLayout;
  vblRight->addWidget(lblInfo);
  vblRight->addWidget(scaScroll);

  trwNodes = new QTreeWidget;
  trwNodes->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
  trwNodes->header()->hide();
  trwNodes->setIndentation(12);
  trwNodes->setColumnCount(1);
  trwNodes->setMaximumWidth(160);
  trwNodes->setSortingEnabled(true);
  trwNodes->sortByColumn(0,Qt::AscendingOrder);
  connect(trwNodes,SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
    SLOT(onCurrentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
  
  QHBoxLayout *hblCentral = new QHBoxLayout;
  hblCentral->addWidget(trwNodes);
  hblCentral->addLayout(vblRight);
  
  dbbButtons = new QDialogButtonBox(Qt::Horizontal);
  dbbButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
  connect(dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
  
  QVBoxLayout *vblMain = new QVBoxLayout;
  vblMain->addLayout(hblCentral);
  vblMain->addWidget(dbbButtons);
  vblMain->setMargin(5);
  
  setLayout(vblMain);
  resize(600,600);
}

OptionsDialog::~OptionsDialog()
{
  emit closed();
  qDeleteAll(FNodes);
}

void OptionsDialog::openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QIcon &AIcon, QWidget *AWidget)
{
  OptionsNode *node = FNodes.value(ANode,NULL);
  if (!node && !ANode.isEmpty() && !AName.isEmpty())
  {
    node = new OptionsNode;
    node->name = AName;
    node->desc = ADescription;
    node->icon = AIcon;
    node->widget = AWidget;
    FNodes.insert(ANode,node);
    
    QTreeWidgetItem *nodeItem = createTreeItem(ANode);
    nodeItem->setText(0,AName);
    nodeItem->setIcon(0,AIcon);
    nodeItem->setWhatsThis(0,ADescription);

    if (AWidget)
    {
      int itemIndex = stwOptions->addWidget(AWidget);
      FItemsStackIndex.insert(nodeItem,itemIndex);
      connect(this,SIGNAL(closed()),AWidget,SLOT(deleteLater()));
    }
  }
}

void OptionsDialog::closeNode(const QString &ANode)
{
  QHash<QString, OptionsNode *>::iterator it = FNodes.begin();
  while (it != FNodes.end())
  {
    if (it.key().startsWith(ANode))
    {
      QTreeWidgetItem *nodeItem = FNodeItems.value(it.key());
      FItemsStackIndex.remove(nodeItem);
      
      //Delete parent TreeItems without widgets
      while (nodeItem->parent() && nodeItem->parent()->childCount()==1 && !FItemsStackIndex.contains(nodeItem->parent()))
      {
        FNodeItems.remove(nodeItem->parent()->data(0,Qt::UserRole).toString());
        nodeItem = nodeItem->parent();
      }

      OptionsNode *node = it.value();
      int widgetIndex = stwOptions->indexOf(node->widget);
      if (widgetIndex > -1)
      {
        QHash<QTreeWidgetItem *, int>::iterator sit = FItemsStackIndex.begin();
        for (;sit != FItemsStackIndex.end();sit++)
          if (sit.value() > widgetIndex)
            sit.value()--;
        stwOptions->removeWidget(node->widget);
      }
      delete node;

      FNodeItems.remove(it.key());
      delete nodeItem;

      it = FNodes.erase(it);
    }
    else
      it++;
  }
}

void OptionsDialog::showNode(const QString &ANode)
{
  QTreeWidgetItem *item = FNodeItems.value(ANode, NULL);
  if (item)
    trwNodes->setCurrentItem(item,0);
  trwNodes->expandAll();
}

QTreeWidgetItem *OptionsDialog::createTreeItem(const QString &ANode)
{
  QString nodeName;
  QString nodeTreeItem;
  QTreeWidgetItem *nodeItem = NULL;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  foreach(nodeTreeItem,nodeTree)
  {
    if (nodeName.isEmpty())
      nodeName = nodeTreeItem;
    else 
      nodeName += "::"+nodeTreeItem;
    
    if (!FNodeItems.contains(nodeName))
    {
      if (nodeItem)
        nodeItem = new QTreeWidgetItem(nodeItem);
      else
        nodeItem = new QTreeWidgetItem(trwNodes);
      nodeItem->setData(0,Qt::UserRole,nodeName);
      nodeItem->setText(0,nodeTreeItem);
      FNodeItems.insert(nodeName,nodeItem);
    }
    else
      nodeItem = FNodeItems.value(nodeName);
  }
  return nodeItem;
}

QString OptionsDialog::nodeFullName(const QString &ANode)
{
  QString fullName;
  QTreeWidgetItem *item = FNodeItems.value(ANode);
  if (item)
  {
    fullName = item->text(0);
    while (item->parent())
    {
      item = item->parent();
      fullName = item->text(0)+"->"+fullName; 
    }
  }
  return fullName;
}

bool OptionsDialog::canExpandVertically(const QWidget *AWidget) const
{
  bool expanding = AWidget->sizePolicy().verticalPolicy() == QSizePolicy::Expanding;
  if (!expanding)
  {
    QObjectList childs = AWidget->children();
    for (int i=0; !expanding && i<childs.count(); i++)
      if (childs.at(i)->isWidgetType())
        expanding = canExpandVertically(qobject_cast<QWidget *>(childs.at(i)));
  }
  return expanding;
}

void OptionsDialog::updateOptionsSize(QWidget *AWidget)
{
  if (AWidget)
  {
    if (!canExpandVertically(AWidget))
      stwOptions->setMaximumHeight(AWidget->sizeHint().height());
    else
      stwOptions->setMaximumHeight(qMax(AWidget->sizeHint().height(),scaScroll->viewport()->size().height()));
  }
}

void OptionsDialog::resizeEvent(QResizeEvent *AEvent)
{
  QDialog::resizeEvent(AEvent);
  updateOptionsSize(stwOptions->currentWidget());
}

void OptionsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  switch(dbbButtons->buttonRole(AButton))
  {
  case QDialogButtonBox::AcceptRole:
    accept(); 
    break;
  case QDialogButtonBox::RejectRole:
    reject(); 
    break;
  case QDialogButtonBox::ApplyRole:
    emit accepted(); 
    break;
  }
}

void OptionsDialog::onCurrentItemChanged(QTreeWidgetItem *ACurrent, QTreeWidgetItem * /*APrevious*/)
{ 
  if (FItemsStackIndex.contains(ACurrent))
  {
    QString node = ACurrent->data(0,Qt::UserRole).toString();
    OptionsNode *nodeOption = FNodes.value(node,NULL);
    if (nodeOption)
    {
      lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(Qt::escape(nodeOption->desc)));
      QWidget *widget = stwOptions->widget(FItemsStackIndex.value(ACurrent));
      stwOptions->setCurrentWidget(widget);
      updateOptionsSize(widget);
      scaScroll->ensureVisible(0,0);
    }
    stwOptions->setVisible(true);
  }
  else if (ACurrent)
  {
    QString node = ACurrent->data(0,Qt::UserRole).toString();
    lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(tr("No Settings Available")));
    stwOptions->setVisible(false);
  }
}

