#include "optionsdialog.h"

#include <QLabel>
#include <QFrame>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QTextDocument>

enum StandardItemRoles {
  SIR_NODE = Qt::UserRole+1,
  SIR_NAME,
  SIR_DESC,
  SIR_ICON,
  SIR_ORDER
};

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
  if (ALeft.data(SIR_ORDER).toInt() != ARight.data(SIR_ORDER).toInt())
    return ALeft.data(SIR_ORDER).toInt() < ARight.data(SIR_ORDER).toInt();
  return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

OptionsDialog::OptionsDialog(QWidget *AParent) : QDialog(AParent)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Options"));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SETTINGS_OPTIONS,0,0,"windowIcon");

  lblInfo = new QLabel(this);
  lblInfo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  lblInfo->setFrameStyle(QFrame::StyledPanel);

  scaScroll = new QScrollArea(this);
  scaScroll->setWidgetResizable(true);
  scaScroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  stwOptions = new QStackedWidget(scaScroll);
  scaScroll->setWidget(stwOptions);

  QVBoxLayout *vblRight = new QVBoxLayout;
  vblRight->addWidget(lblInfo);
  vblRight->addWidget(scaScroll);

  trvNodes = new QTreeView(this);
  trvNodes->header()->hide();
  trvNodes->setIndentation(12);
  trvNodes->setMaximumWidth(160);
  trvNodes->setSortingEnabled(true);
  trvNodes->sortByColumn(0,Qt::AscendingOrder);
  trvNodes->setEditTriggers(QAbstractItemView::NoEditTriggers);
  trvNodes->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);

  simNodes = new QStandardItemModel(trvNodes);
  simNodes->setColumnCount(1);

  FProxyModel = new SortFilterProxyModel(trvNodes);
  FProxyModel->setSortLocaleAware(true);
  FProxyModel->setDynamicSortFilter(true);
  FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
  FProxyModel->setSourceModel(simNodes);
  
  trvNodes->setModel(FProxyModel);
  connect(trvNodes->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));

  QHBoxLayout *hblCentral = new QHBoxLayout;
  hblCentral->addWidget(trvNodes);
  hblCentral->addLayout(vblRight);

  dbbButtons = new QDialogButtonBox(Qt::Horizontal,this);
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

void OptionsDialog::openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIcon, int AOrder, QWidget *AWidget)
{
  OptionsNode *node = FNodes.value(ANode);
  if (!node && !ANode.isEmpty() && !AName.isEmpty())
  {
    node = new OptionsNode;
    node->name = AName;
    node->desc = ADescription;
    node->icon = AIcon;
    node->order = AOrder;
    node->widget = AWidget;
    FNodes.insert(ANode,node);

    QStandardItem *nodeItem = createNodeItem(ANode);
    nodeItem->setText(AName);
    nodeItem->setWhatsThis(ADescription);
    nodeItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AIcon));
    nodeItem->setData(AName,SIR_NAME);
    nodeItem->setData(ADescription,SIR_DESC);
    nodeItem->setData(AOrder,SIR_ORDER);

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
      QStandardItem *nodeItem = FNodeItems.value(it.key());
      FItemsStackIndex.remove(nodeItem);

      //Delete parent items without widgets
      while (nodeItem->parent() && nodeItem->parent()->rowCount()==1 && !FItemsStackIndex.contains(nodeItem->parent()))
      {
        FNodeItems.remove(nodeItem->parent()->data(SIR_NODE).toString());
        nodeItem = nodeItem->parent();
      }

      OptionsNode *node = it.value();
      int widgetIndex = stwOptions->indexOf(node->widget);
      if (widgetIndex > -1)
      {
        QHash<QStandardItem *, int>::iterator sit = FItemsStackIndex.begin();
        for (;sit != FItemsStackIndex.end();sit++)
          if (sit.value() > widgetIndex)
            sit.value()--;
        stwOptions->removeWidget(node->widget);
      }
      delete node;

      FNodeItems.remove(it.key());
      nodeItem->parent()->removeRow(nodeItem->row());

      it = FNodes.erase(it);
    }
    else
      it++;
  }
}

void OptionsDialog::showNode(const QString &ANode)
{
  QStandardItem *item = FNodeItems.value(ANode, NULL);
  if (item)
    trvNodes->setCurrentIndex(FProxyModel->mapFromSource(simNodes->indexFromItem(item)));
  trvNodes->expandAll();
}

QStandardItem *OptionsDialog::createNodeItem(const QString &ANode)
{
  QString nodeName;
  QStandardItem *nodeItem = NULL;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  foreach(QString nodeTreeItem,nodeTree)
  {
    if (nodeName.isEmpty())
      nodeName = nodeTreeItem;
    else
      nodeName += "::"+nodeTreeItem;

    if (!FNodeItems.contains(nodeName))
    {
      if (nodeItem)
      {
        QStandardItem *newItem = new QStandardItem(nodeTreeItem);
        nodeItem->appendRow(newItem);
        nodeItem = newItem;
      }
      else
      {
        nodeItem = new QStandardItem(nodeTreeItem);
        simNodes->appendRow(nodeItem);
      }
      nodeItem->setData(nodeName,SIR_NODE);
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
  QStandardItem *item = FNodeItems.value(ANode);
  if (item)
  {
    fullName = item->text();
    while (item->parent())
    {
      item = item->parent();
      fullName = item->text()+"->"+fullName;
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

void OptionsDialog::showEvent(QShowEvent *AEvent)
{
  QDialog::showEvent(AEvent);
  trvNodes->setCurrentIndex(FProxyModel->mapFromSource(simNodes->indexFromItem(simNodes->item(0))));
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
  default:
    break;
  }
}

void OptionsDialog::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &/*APrevious*/)
{
  QStandardItem *curItem = simNodes->itemFromIndex(FProxyModel->mapToSource(ACurrent));
  if (FItemsStackIndex.contains(curItem))
  {
    QString node = curItem->data(SIR_NODE).toString();
    lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(Qt::escape(curItem->data(SIR_DESC).toString())));
    QWidget *widget = stwOptions->widget(FItemsStackIndex.value(curItem));
    stwOptions->setCurrentWidget(widget);
    updateOptionsSize(widget);
    scaScroll->ensureVisible(0,0);
    stwOptions->setVisible(true);
  }
  else if (curItem)
  {
    QString node = curItem->data(SIR_NODE).toString();
    lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(tr("No Settings Available")));
    stwOptions->setVisible(false);
  }
}
