#include "optionsdialog.h"

#include <QLabel>
#include <QFrame>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QApplication>
#include <QTextDocument>
#include "settingsplugin.h"

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

OptionsDialog::OptionsDialog(SettingsPlugin *ASessingsPlugin, QWidget *AParent) : QDialog(AParent)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Options"));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SETTINGS_OPTIONS,0,0,"windowIcon");

  FSettingsPlugin = ASessingsPlugin;

  lblInfo = new QLabel(this);
  lblInfo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  lblInfo->setFrameStyle(QFrame::Box);
  lblInfo->setFrameShadow(QFrame::Sunken);

  scaScroll = new QScrollArea(this);
  scaScroll->setWidgetResizable(true);
  scaScroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

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
  trvNodes->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);

  simNodes = new QStandardItemModel(trvNodes);
  simNodes->setColumnCount(1);

  FProxyModel = new SortFilterProxyModel(simNodes);
  FProxyModel->setSourceModel(simNodes);
  FProxyModel->setSortLocaleAware(true);
  FProxyModel->setDynamicSortFilter(true);
  FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

  QHBoxLayout *hblCentral = new QHBoxLayout;
  hblCentral->addWidget(trvNodes);
  hblCentral->addLayout(vblRight);

  dbbButtons = new QDialogButtonBox(Qt::Horizontal,this);
  dbbButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
  connect(dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

  QVBoxLayout *vblMain = new QVBoxLayout;
  vblMain->addLayout(hblCentral);
  vblMain->addWidget(dbbButtons);
  vblMain->setMargin(3);

  setLayout(vblMain);
  resize(600,600);

  trvNodes->setModel(FProxyModel);
  connect(trvNodes->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));
}

OptionsDialog::~OptionsDialog()
{
  emit closed();
  qDeleteAll(FNodes);
}

void OptionsDialog::openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIcon, int AOrder)
{
  OptionsNode *node = FNodes.value(ANode);
  if (!node && !ANode.isEmpty() && !AName.isEmpty())
  {
    node = new OptionsNode;
    node->name = AName;
    node->desc = ADescription;
    node->icon = AIcon;
    node->order = AOrder;
    FNodes.insert(ANode,node);

    QStandardItem *nodeItem = createNodeItem(ANode);
    nodeItem->setText(AName);
    nodeItem->setWhatsThis(ADescription);
    nodeItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AIcon));
    nodeItem->setData(AName,SIR_NAME);
    nodeItem->setData(ADescription,SIR_DESC);
    nodeItem->setData(AOrder,SIR_ORDER);
  }
}

void OptionsDialog::closeNode(const QString &ANode)
{
  QMap<QString, OptionsNode *>::iterator it = FNodes.begin();
  while (it != FNodes.end())
  {
    if (it.key().startsWith(ANode))
    {
      QStandardItem *nodeItem = FNodeItems.value(it.key());
      FItemWidget.remove(nodeItem);

      //Delete parent items without widgets
      while (nodeItem->parent() && nodeItem->parent()->rowCount()==1 && !FItemWidget.contains(nodeItem->parent()))
      {
        FNodeItems.remove(nodeItem->parent()->data(SIR_NODE).toString());
        nodeItem = nodeItem->parent();
      }

      FNodeItems.remove(it.key());
      if (nodeItem->parent())
        nodeItem->parent()->removeRow(nodeItem->row());
      else
        simNodes->takeItem(nodeItem->row());

      delete it.value();
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

void OptionsDialog::showEvent(QShowEvent *AEvent)
{
  QDialog::showEvent(AEvent);
  trvNodes->setCurrentIndex(FProxyModel->mapFromSource(simNodes->indexFromItem(simNodes->item(0))));
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

  if (curItem && !FItemWidget.contains(curItem))
  {
    QWidget *widget = FSettingsPlugin->createNodeWidget(curItem->data(SIR_NODE).toString());
    if (!canExpandVertically(widget))
      widget->setMaximumHeight(widget->sizeHint().height());
    FItemWidget.insert(curItem,widget);
    connect(this,SIGNAL(closed()),widget,SLOT(deleteLater()));
  }

  QWidget *curWidget = FItemWidget.value(curItem, NULL);
  if (curWidget)
  {
    QString node = curItem->data(SIR_NODE).toString();
    lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(Qt::escape(curItem->data(SIR_DESC).toString())));
    scaScroll->takeWidget();
    scaScroll->setWidget(curWidget);
    int addWidth = curWidget->sizeHint().width() - scaScroll->width();
    addWidth += QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    addWidth += QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth)*2;
    if (addWidth > 0)
      resize(width()+addWidth,height());
  }
  else if (curItem)
  {
    QString node = curItem->data(SIR_NODE).toString();
    lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(tr("No Settings Available")));
    scaScroll->takeWidget();
  }
}
