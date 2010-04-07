#include "optionsdialog.h"

#include <QHeaderView>
#include <QTextDocument>
#include "settingsplugin.h"

#define BDI_OPTIONS_DIALOG_GEOMETRY     "Settings::OptionsDialogGeometry"
#define BDI_OPTIONS_DIALOG_SPLITTER     "Settings::OptionsDialogSplitter"

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
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Options"));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_OPTIONS_DIALOG,0,0,"windowIcon");
  
  delete ui.scaScroll->takeWidget();
  ui.trvNodes->sortByColumn(0,Qt::AscendingOrder);

  FSettingsPlugin = ASessingsPlugin;
  ISettings *settings = FSettingsPlugin->settingsForPlugin(SETTINGS_UUID);
  restoreGeometry(settings->loadBinaryData(BDI_OPTIONS_DIALOG_GEOMETRY));
  if (!ui.sprSplitter->restoreState(settings->loadBinaryData(BDI_OPTIONS_DIALOG_SPLITTER)))
    ui.sprSplitter->setSizes(QList<int>() << 150 << 450);

  FNodesModel = new QStandardItemModel(ui.trvNodes);
  FNodesModel->setColumnCount(1);

  FProxyModel = new SortFilterProxyModel(FNodesModel);
  FProxyModel->setSourceModel(FNodesModel);
  FProxyModel->setSortLocaleAware(true);
  FProxyModel->setDynamicSortFilter(true);
  FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

  ui.trvNodes->setModel(FProxyModel);
  connect(ui.trvNodes->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));
  connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
}

OptionsDialog::~OptionsDialog()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(SETTINGS_UUID);
  settings->saveBinaryData(BDI_OPTIONS_DIALOG_GEOMETRY, saveGeometry());
  settings->saveBinaryData(BDI_OPTIONS_DIALOG_SPLITTER,ui.sprSplitter->saveState());

  emit closed();
}

void OptionsDialog::openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIcon, int AOrder)
{
  if (!FNodeItems.contains(ANode) && !ANode.isEmpty() && !AName.isEmpty())
  {
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
  foreach(QString node, FNodeItems.keys())
  {
    if (node.startsWith(ANode))
    {
      QStandardItem *nodeItem = FNodeItems.value(node);
      if (nodeItem->parent())
        nodeItem->parent()->removeRow(nodeItem->row());
      else
        delete FNodesModel->takeItem(nodeItem->row());

      FItemWidget.remove(nodeItem);
      FNodeItems.remove(node);
    }
  }
}

void OptionsDialog::showNode(const QString &ANode)
{
  QStandardItem *item = FNodeItems.value(ANode, NULL);
  if (item)
    ui.trvNodes->setCurrentIndex(FProxyModel->mapFromSource(FNodesModel->indexFromItem(item)));
  else
    ui.trvNodes->setCurrentIndex(FProxyModel->mapFromSource(FNodesModel->indexFromItem(FNodesModel->item(0))));
  ui.trvNodes->expandAll();
}

QStandardItem *OptionsDialog::createNodeItem(const QString &ANode)
{
  QString nodeName;
  QStandardItem *nodeItem = NULL;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  foreach(QString nodeTreeItem, nodeTree)
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
        FNodesModel->appendRow(nodeItem);
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

void OptionsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  switch(ui.dbbButtons->buttonRole(AButton))
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

void OptionsDialog::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious)
{
  Q_UNUSED(APrevious);
  QStandardItem *curItem = FNodesModel->itemFromIndex(FProxyModel->mapToSource(ACurrent));

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
    ui.lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(Qt::escape(curItem->data(SIR_DESC).toString())));
    ui.scaScroll->takeWidget();
    ui.scaScroll->setWidget(curWidget);
  }
  else if (curItem)
  {
    QString node = curItem->data(SIR_NODE).toString();
    ui.lblInfo->setText(QString("<b>%1</b><br>%2").arg(Qt::escape(nodeFullName(node))).arg(tr("No Settings Available")));
    ui.scaScroll->takeWidget();
  }
}
