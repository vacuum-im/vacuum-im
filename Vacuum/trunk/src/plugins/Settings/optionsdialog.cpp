#include "optionsdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QHeaderView>


OptionsDialog::OptionsDialog(QWidget *AParent)
  : QDialog(AParent)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  QLabel *lblInfo = new QLabel("Node name\nNode description");
  lblInfo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  lblInfo->setFrameStyle(QFrame::StyledPanel);
  QScrollArea *scaScroll = new QScrollArea;
  scaScroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  scaScroll->setWidgetResizable(true);
  scaScroll->setFrameShape(QFrame::NoFrame);
  stwOptions = new QStackedWidget;
  scaScroll->setWidget(stwOptions);
  QVBoxLayout *vblRight = new QVBoxLayout;
  vblRight->addWidget(lblInfo);
  vblRight->addWidget(scaScroll);
  trwNodes = new QTreeWidget;
  trwNodes->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
  trwNodes->header()->hide();
  trwNodes->setColumnCount(1);
  trwNodes->setMaximumWidth(120);
  connect(trwNodes,SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
    SLOT(onCurrentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
  QHBoxLayout *hblCentral = new QHBoxLayout;
  hblCentral->addWidget(trwNodes);
  hblCentral->addLayout(vblRight);
  dbbButtons = new QDialogButtonBox(Qt::Horizontal);
  dbbButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
  connect(dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
  QVBoxLayout *vblMain = new QVBoxLayout;
  vblMain->addLayout(hblCentral);
  vblMain->addWidget(dbbButtons);
  setLayout(vblMain);
  setWindowTitle(tr("Options"));
  resize(550,500);
}

OptionsDialog::~OptionsDialog()
{
  qDeleteAll(FNodes);
}

void OptionsDialog::openNode(const QString &ANode, const QString &AName,  
                             const QString &ADescription, const QIcon &AIcon, QWidget *AWidget)
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
      QTreeWidgetItem *nodeItem = FNodesItems.value(it.key());
      FItemsStackIndex.remove(nodeItem);
      
      //Delete parent TreeItems without widgets
      while (nodeItem->parent() && !FItemsStackIndex.contains(nodeItem->parent()))
      {
        FNodesItems.remove(nodeItem->data(0,Qt::UserRole).toString());
        nodeItem = nodeItem->parent();
      }

      OptionsNode *node = it.value();
      stwOptions->removeWidget(node->widget);
      delete node;

      FNodesItems.remove(it.key());
      delete nodeItem;

      it = FNodes.erase(it);
    }
    else
      it++;
  }
}

void OptionsDialog::showNode( const QString &ANode )
{
  QTreeWidgetItem *item = FNodesItems.value(ANode, NULL);
  if (item)
    trwNodes->setCurrentItem(item,0);
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
    if (!FNodesItems.contains(nodeName))
    {
      if (nodeItem)
        nodeItem = new QTreeWidgetItem(nodeItem);
      else
        nodeItem = new QTreeWidgetItem(trwNodes);
      nodeItem->setData(0,Qt::UserRole,nodeName);
      nodeItem->setText(0,nodeTreeItem);
      trwNodes->expandItem(nodeItem);
      FNodesItems.insert(nodeName,nodeItem);
    }
    else
      nodeItem = FNodesItems.value(nodeName);
  }
  return nodeItem;
}

void OptionsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  switch(dbbButtons->buttonRole(AButton))
  {
  case QDialogButtonBox::AcceptRole:
    accept(); break;
  case QDialogButtonBox::RejectRole:
    reject(); break;
  case QDialogButtonBox::ApplyRole:
    emit accepted(); break;
  }
}

void OptionsDialog::onCurrentItemChanged(QTreeWidgetItem *ACurrent, QTreeWidgetItem *)
{ 
  QTreeWidgetItem *currentItem = ACurrent;
  while (!FItemsStackIndex.contains(currentItem) && currentItem->childCount()>0)
    currentItem = currentItem->child(0);

  if (FItemsStackIndex.contains(currentItem))
    stwOptions->setCurrentIndex(FItemsStackIndex.value(currentItem));
}

