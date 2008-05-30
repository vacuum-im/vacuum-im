#include "discoitemswindow.h"

#include <QHeaderView>
#include <QLineEdit>

#define CNAME                   0
#define CJID                    1
#define CNODE                   2

#define MAX_ITEMS_FOR_REQUEST   20

#define IN_ARROW_LEFT           "psi/arrowLeft"
#define IN_ARROW_RIGHT          "psi/arrowRight"
#define IN_USE_CACH             "psi/browse"
#define IN_DISCOVER             "psi/jabber"
#define IN_RELOAD               "psi/reload"
#define IN_DISCO                "psi/disco"
#define IN_ADDCONTACT           "psi/addContact"
#define IN_VCARD                "psi/vCard"
#define IN_INFO                 "psi/statusmsg"

DiscoItemsWindow::DiscoItemsWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QWidget *AParent) : QMainWindow(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowIcon(Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_DISCO));
  setWindowTitle(tr("Service Discovery"));

  FVCardPlugin = NULL;
  FRosterChanger = NULL;

  FDiscovery = ADiscovery;
  FStreamJid = AStreamJid;
  FCurrentStep = -1;

  FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);

  FActionsBarChanger = new ToolBarChanger(new QToolBar(this));
  FActionsBarChanger->setManageVisibility(false);
  FActionsBarChanger->toolBar()->setIconSize(this->iconSize());
  FActionsBarChanger->toolBar()->setOrientation(Qt::Vertical);
  FActionsBarChanger->toolBar()->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  FActionsBarChanger->toolBar()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  ui.grbActions->setLayout(new QVBoxLayout);
  ui.grbActions->layout()->setMargin(2);
  ui.grbActions->layout()->addWidget(FActionsBarChanger->toolBar());

  connect(ui.cmbJid->lineEdit(),SIGNAL(returnPressed()),SLOT(onComboReturnPressed()));
  connect(ui.cmbNode->lineEdit(),SIGNAL(returnPressed()),SLOT(onComboReturnPressed()));

  ui.trwItems->header()->setStretchLastSection(false);
  ui.trwItems->header()->setResizeMode(CNAME,QHeaderView::ResizeToContents);
  ui.trwItems->header()->setResizeMode(CJID,QHeaderView::Stretch);
  ui.trwItems->header()->setResizeMode(CNODE,QHeaderView::Stretch);
  ui.trwItems->sortByColumn(CNAME,Qt::AscendingOrder);
  connect(ui.trwItems,SIGNAL(itemExpanded(QTreeWidgetItem *)),SLOT(onTreeItemExpanded(QTreeWidgetItem *)));
  connect(ui.trwItems,SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
    SLOT(onCurrentTreeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
  connect(ui.trwItems,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onTreeItemContextMenu(const QPoint &)));

  connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
  connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsReceived(const IDiscoItems &)));
  connect(FDiscovery->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),SLOT(onStreamJidChanged(const Jid &, const Jid &)));

  initialize();
  createToolBarActions();
}

DiscoItemsWindow::~DiscoItemsWindow()
{

}

void DiscoItemsWindow::setUseCache(bool AUse)
{
  FUseCache->setChecked(AUse);
}

void DiscoItemsWindow::discover(const Jid AContactJid, const QString &ANode)
{
  ui.cmbJid->setEditText(AContactJid.full());
  ui.cmbNode->setEditText(ANode);
  while (ui.trwItems->topLevelItemCount() > 0)
    destroyTreeItem(ui.trwItems->topLevelItem(0));

  QPair<Jid,QString>step(AContactJid,ANode);
  if (step != FDiscoverySteps.value(FCurrentStep))
    FDiscoverySteps.insert(++FCurrentStep,step);

  if (ui.cmbJid->findText(ui.cmbJid->currentText()) < 0)
    ui.cmbJid->addItem(ui.cmbJid->currentText());
  if (ui.cmbNode->findText(ui.cmbNode->currentText()) < 0)
    ui.cmbNode->addItem(ui.cmbNode->currentText());

  emit discoverChanged(AContactJid,ANode);

  IDiscoItem ditem;
  ditem.itemJid = AContactJid;
  ditem.node = ANode;
  QTreeWidgetItem *treeItem = createTreeItem(ditem,NULL);

  if (!useCache() || isNeedRequestInfo(AContactJid,ANode))
    requestDiscoInfo(AContactJid,ANode);
  else
    updateDiscoInfo(FDiscovery->discoInfo(AContactJid,ANode));

  if (!useCache() || isNeedRequestItems(AContactJid,ANode))
    requestDiscoItems(AContactJid,ANode);
  else
    updateDiscoItems(FDiscovery->discoItems(AContactJid,ANode));

  ui.trwItems->setCurrentItem(treeItem);
  treeItem->setExpanded(true);
}

void DiscoItemsWindow::initialize()
{
  IPlugin *plugin = FDiscovery->pluginManager()->getPlugins("IRosterChanger").value(0,NULL);
  if (plugin)
    FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

  plugin = FDiscovery->pluginManager()->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
}

bool DiscoItemsWindow::isNeedRequestInfo(const Jid AContactJid, const QString &ANode) const
{
  return !FDiscovery->hasDiscoInfo(AContactJid,ANode) || FDiscovery->discoInfo(AContactJid,ANode).error.code>0; 
}

bool DiscoItemsWindow::isNeedRequestItems(const Jid AContactJid, const QString &ANode) const
{
  return !FDiscovery->hasDiscoItems(AContactJid,ANode) || FDiscovery->discoItems(AContactJid,ANode).error.code>0; 
}

void DiscoItemsWindow::requestDiscoInfo(const Jid AContactJid, const QString &ANode)
{
  QTreeWidgetItem *treeItem = FTreeItems.value(AContactJid).value(ANode);
  if (treeItem!=NULL && !treeItem->isDisabled() && FDiscovery->requestDiscoInfo(FStreamJid,AContactJid,ANode))
  {
    treeItem->setData(0,DDR_REQUEST_INFO,true);
    treeItem->setIcon(0,FReloadCurrent->icon());
  }
}

void DiscoItemsWindow::requestDiscoItems(const Jid AContactJid, const QString &ANode)
{
  QTreeWidgetItem *treeItem = FTreeItems.value(AContactJid).value(ANode);
  if (treeItem!=NULL && !treeItem->isDisabled() && FDiscovery->requestDiscoItems(FStreamJid,AContactJid,ANode))
  {
    for(int i=0; i<treeItem->childCount(); i++)
      treeItem->child(i)->setDisabled(true);
    treeItem->setData(0,DDR_REQUEST_ITEMS,true);
    treeItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
  }
}

QTreeWidgetItem *DiscoItemsWindow::createTreeItem(const IDiscoItem &ADiscoItem, QTreeWidgetItem *AParent)
{
  QTreeWidgetItem *treeItem = new QTreeWidgetItem;
  treeItem->setText(CNAME, ADiscoItem.name.isEmpty() ? ADiscoItem.itemJid.full() : ADiscoItem.name);
  treeItem->setText(CJID, ADiscoItem.itemJid.full());
  treeItem->setText(CNODE, ADiscoItem.node);
  treeItem->setToolTip(CNAME,ADiscoItem.name);
  treeItem->setToolTip(CJID,ADiscoItem.itemJid.hFull());
  treeItem->setToolTip(CNODE,ADiscoItem.node);
  treeItem->setData(0,DDR_STREAMJID,FStreamJid.full());
  treeItem->setData(0,DDR_JID,ADiscoItem.itemJid.full());
  treeItem->setData(0,DDR_NODE,ADiscoItem.node);
  treeItem->setData(0,DDR_NAME,ADiscoItem.name);
  treeItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
  FTreeItems[ADiscoItem.itemJid].insert(ADiscoItem.node,treeItem);
  if (AParent)
    AParent->addChild(treeItem);
  else
    ui.trwItems->addTopLevelItem(treeItem);
  emit treeItemCreated(treeItem);
  return treeItem;
}


void DiscoItemsWindow::updateDiscoInfo(const IDiscoInfo &ADiscoInfo)
{
  QTreeWidgetItem *treeItem = FTreeItems.value(ADiscoInfo.contactJid).value(ADiscoInfo.node);
  if (treeItem)
  {
    treeItem->setData(0,DDR_REQUEST_INFO,false);

    if (treeItem->parent() == NULL && !ADiscoInfo.identity.value(0).name.isEmpty())
    {
      treeItem->setText(CNAME, ADiscoInfo.identity.value(0).name);
      treeItem->setData(0,DDR_NAME,ADiscoInfo.identity.value(0).name);
    }

    treeItem->setIcon(CNAME,FDiscovery->discoInfoIcon(ADiscoInfo));

    QString toolTip; 
    if (ADiscoInfo.error.code < 0)
    {
      if (!ADiscoInfo.identity.isEmpty())
      {
        toolTip+=tr("<li><b>Identity:</b></li>");
        foreach(IDiscoIdentity identity, ADiscoInfo.identity)
          toolTip+=tr("<li>%1 (Category: '%2'; Type: '%3')</li>").arg(identity.name).arg(identity.category).arg(identity.type);
      }
      
      if (!ADiscoInfo.features.isEmpty())
      {
        QStringList features = ADiscoInfo.features;
        qSort(features);
        toolTip+=tr("<li><b>Features:</b></li>");
        foreach(QString feature, features)
        {
          IDiscoFeature dfeature = FDiscovery->discoFeature(feature);
          toolTip+=QString("<li>%1</li>").arg(dfeature.name.isEmpty() ? feature : dfeature.name);
        }
      }
    }
    else
      toolTip+=tr("Error %1: %2").arg(ADiscoInfo.error.code).arg(ADiscoInfo.error.message);

    treeItem->setToolTip(CNAME,toolTip);

    if(treeItem == ui.trwItems->currentItem())
      updateActionsBar();

    emit treeItemChanged(treeItem);
  }
}

void DiscoItemsWindow::updateDiscoItems(const IDiscoItems &ADiscoItems)
{
  QTreeWidgetItem *treeItem = FTreeItems.value(ADiscoItems.contactJid).value(ADiscoItems.node);
  if (treeItem)
  {
    treeItem->setData(0,DDR_REQUEST_ITEMS,false);

    QList<QTreeWidgetItem *> curItems;
    foreach(IDiscoItem ditem, ADiscoItems.items)
    {
      QTreeWidgetItem *curItem = FTreeItems.value(ditem.itemJid).value(ditem.node);
      if (curItem == NULL)
      {
        curItem = createTreeItem(ditem,treeItem);
        if (useCache() && !isNeedRequestInfo(ditem.itemJid,ditem.node))
          updateDiscoInfo(FDiscovery->discoInfo(ditem.itemJid,ditem.node));
        else if (ADiscoItems.items.count() <= MAX_ITEMS_FOR_REQUEST)
          requestDiscoInfo(ditem.itemJid,ditem.node);
        else
          curItem->setIcon(CNAME,FDiscovery->discoItemIcon(ditem));
        if (!isNeedRequestItems(ditem.itemJid,ditem.node))
          updateDiscoItems(FDiscovery->discoItems(ditem.itemJid,ditem.node));
      }
      else
        curItem->setDisabled(false);
      curItems.append(curItem);
    }

    int index = 0;
    while (index < treeItem->childCount())
    {
      if (!curItems.contains(treeItem->child(index)))
        destroyTreeItem(treeItem->takeChild(index));
      else
        index++;
    }

    if (ADiscoItems.error.code > 0)
    {
      ui.trwItems->collapseItem(treeItem);
      treeItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
    else
      treeItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

    emit treeItemChanged(treeItem);
  }
}

void DiscoItemsWindow::destroyTreeItem(QTreeWidgetItem *ATreeItem)
{
  QList<QTreeWidgetItem *> childItems = ATreeItem->takeChildren();
  foreach(QTreeWidgetItem *childItem,childItems)
    destroyTreeItem(childItem);

  Jid itemJid = ATreeItem->data(0,DDR_JID).toString();
  QString itemNode = ATreeItem->data(0,DDR_NODE).toString();
  if (FTreeItems.value(itemJid).count() > 1)
    FTreeItems[itemJid].remove(itemNode);
  else
    FTreeItems.remove(itemJid);
  emit treeItemDestroyed(ATreeItem);
  delete ATreeItem;
}

void DiscoItemsWindow::createToolBarActions()
{
  FMoveBack = new Action(FToolBarChanger);
  FMoveBack->setText(tr("Back"));
  FMoveBack->setIcon(SYSTEM_ICONSETFILE,IN_ARROW_LEFT);
  FToolBarChanger->addAction(FMoveBack,AG_DIWT_DISCOVERY_NAVIGATE,false);
  connect(FMoveBack,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FMoveForward = new Action(FToolBarChanger);
  FMoveForward->setText(tr("Forward"));
  FMoveForward->setIcon(SYSTEM_ICONSETFILE,IN_ARROW_RIGHT);
  FToolBarChanger->addAction(FMoveForward,AG_DIWT_DISCOVERY_NAVIGATE,false);
  connect(FMoveForward,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FUseCache = new Action(FToolBarChanger);
  FUseCache->setText(tr("Use Cache"));
  FUseCache->setIcon(SYSTEM_ICONSETFILE,IN_USE_CACH);
  FUseCache->setCheckable(true);
  FUseCache->setChecked(true);
  FToolBarChanger->addAction(FUseCache,AG_DIWT_DISCOVERY_DEFACTIONS,false);
  connect(FUseCache,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FDiscoverCurrent = new Action(FToolBarChanger);
  FDiscoverCurrent->setText(tr("Discover"));
  FDiscoverCurrent->setIcon(SYSTEM_ICONSETFILE,IN_DISCOVER);
  FToolBarChanger->addAction(FDiscoverCurrent,AG_DIWT_DISCOVERY_DEFACTIONS,false);
  connect(FDiscoverCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FReloadCurrent = new Action(FToolBarChanger);
  FReloadCurrent->setText(tr("Reload"));
  FReloadCurrent->setIcon(SYSTEM_ICONSETFILE,IN_RELOAD);
  FToolBarChanger->addAction(FReloadCurrent,AG_DIWT_DISCOVERY_DEFACTIONS,false);
  connect(FReloadCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FDiscoInfo = new Action(FToolBarChanger);
  FDiscoInfo->setText(tr("Disco info"));
  FDiscoInfo->setIcon(SYSTEM_ICONSETFILE,IN_INFO);
  FToolBarChanger->addAction(FDiscoInfo,AG_DIWT_DISCOVERY_ACTIONS,false);
  connect(FDiscoInfo,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FAddContact = new Action(FToolBarChanger);
  FAddContact->setText(tr("Add Contact"));
  FAddContact->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
  FToolBarChanger->addAction(FAddContact,AG_DIWT_DISCOVERY_ACTIONS,false);
  connect(FAddContact,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FShowVCard = new Action(FToolBarChanger);
  FShowVCard->setText(tr("vCard"));
  FShowVCard->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
  FToolBarChanger->addAction(FShowVCard,AG_DIWT_DISCOVERY_ACTIONS,false);
  connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
  
  updateToolBarActions();
}

void DiscoItemsWindow::updateToolBarActions()
{
  FMoveBack->setEnabled(FCurrentStep > 0);
  FMoveForward->setEnabled(FCurrentStep < FDiscoverySteps.count()-1);
  FDiscoverCurrent->setEnabled(ui.trwItems->currentItem() && ui.trwItems->currentItem()->parent());
  FReloadCurrent->setEnabled(ui.trwItems->currentItem());
  FDiscoInfo->setEnabled(ui.trwItems->currentItem());
  FAddContact->setEnabled(FRosterChanger != NULL);
  FShowVCard->setEnabled(FVCardPlugin != NULL);
}

void DiscoItemsWindow::updateActionsBar()
{
  qDeleteAll(FActionsBarChanger->actions(AG_DIWT_DISCOVERY_FEATURE_ACTIONS));
  QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
  if (treeItem)
  {
    IDiscoInfo dinfo = FDiscovery->discoInfo(treeItem->data(0,DDR_JID).toString(),treeItem->data(0,DDR_NODE).toString());
    foreach(QString feature, dinfo.features)
    {
      Action *action = FDiscovery->createFeatureAction(FStreamJid,feature,dinfo,this);
      if (action)
      {
        QToolButton *button = FActionsBarChanger->addToolButton(action,AG_DIWT_DISCOVERY_FEATURE_ACTIONS,true);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      }
    }
  }
}

void DiscoItemsWindow::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
  updateDiscoInfo(ADiscoInfo);
}

void DiscoItemsWindow::onDiscoItemsReceived(const IDiscoItems &ADiscoItems)
{
  updateDiscoItems(ADiscoItems);
}

void DiscoItemsWindow::onTreeItemContextMenu(const QPoint &APos)
{
  QTreeWidgetItem *treeItem = ui.trwItems->itemAt(APos);
  if (treeItem)
  {
    ui.trwItems->setCurrentItem(treeItem,ui.trwItems->columnAt(APos.x()));
    Menu *menu = new Menu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose,true);

    menu->addAction(FDiscoverCurrent,AG_DIWT_DISCOVERY_DEFACTIONS,false);
    menu->addAction(FReloadCurrent,AG_DIWT_DISCOVERY_DEFACTIONS,false);
    menu->addAction(FDiscoInfo,AG_DIWT_DISCOVERY_ACTIONS,false);
    menu->addAction(FAddContact,AG_DIWT_DISCOVERY_ACTIONS,false);
    menu->addAction(FShowVCard,AG_DIWT_DISCOVERY_ACTIONS,false);
    
    IDiscoInfo dinfo = FDiscovery->discoInfo(treeItem->data(0,DDR_JID).toString(),treeItem->data(0,DDR_NODE).toString());
    foreach(QString feature, dinfo.features)
    {
      Action *action = FDiscovery->createFeatureAction(FStreamJid,feature,dinfo,menu);
      if (action)
        menu->addAction(action,AG_DIWT_DISCOVERY_FEATURE_ACTIONS,false);
    }
    
    emit treeItemContextMenu(treeItem,menu);

    menu->popup(ui.trwItems->viewport()->mapToGlobal(APos));
  }
}

void DiscoItemsWindow::onTreeItemExpanded(QTreeWidgetItem *AItem)
{
  if (!AItem->data(0,DDR_REQUEST_ITEMS).toBool())
  {
    Jid itemJid = AItem->data(0,DDR_JID).toString();
    QString itemNode = AItem->data(0,DDR_NODE).toString();
    if (!useCache())
      requestDiscoItems(itemJid,itemNode);
    else if(isNeedRequestItems(itemJid,itemNode))
      requestDiscoItems(itemJid,itemNode);
    else if (AItem->childCount()==0)
      updateDiscoItems(FDiscovery->discoItems(itemJid,itemNode));
  }
}

void DiscoItemsWindow::onCurrentTreeItemChanged(QTreeWidgetItem *ACurrent, QTreeWidgetItem *APrevious)
{
  if (ACurrent != APrevious)
  {
    if (ACurrent)
    {
      Jid itemJid = ACurrent->data(0,DDR_JID).toString();
      QString itemNode = ACurrent->data(0,DDR_NODE).toString();
      if (isNeedRequestInfo(itemJid,itemNode))
        requestDiscoInfo(itemJid,itemNode);
    }
    updateToolBarActions();
    updateActionsBar();
    emit treeItemSelected(ACurrent,APrevious);
  }
}

void DiscoItemsWindow::onToolBarActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action == FMoveBack)
  {
    if (FCurrentStep > 0)
    {
      QPair<Jid,QString> step = FDiscoverySteps.at(--FCurrentStep);
      discover(step.first,step.second);
    }
  }
  else if (action == FMoveForward)
  {
    if (FCurrentStep < FDiscoverySteps.count()-1)
    {
      QPair<Jid,QString> step = FDiscoverySteps.at(++FCurrentStep);
      discover(step.first,step.second);
    }
  }
  else if (action == FDiscoverCurrent)
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem && treeItem->parent())
    {
      Jid itemJid = treeItem->data(0,DDR_JID).toString();
      QString itemNode = treeItem->data(0,DDR_NODE).toString();
      discover(itemJid,itemNode);
    }
  }
  else if (action == FReloadCurrent)
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem)
    {
      Jid itemJid = treeItem->data(0,DDR_JID).toString();
      QString itemNode = treeItem->data(0,DDR_NODE).toString();
      requestDiscoInfo(itemJid,itemNode);
      requestDiscoItems(itemJid,itemNode);
      IDiscoItems ditems = FDiscovery->discoItems(itemJid,itemNode);
      foreach(IDiscoItem ditem,ditems.items)
      {
        IDiscoInfo ditemInfo = FDiscovery->discoInfo(ditem.itemJid,ditem.node);
        if (ditemInfo.error.code > 0)
        {
          if (ditems.items.count() <= MAX_ITEMS_FOR_REQUEST)
            requestDiscoInfo(ditem.itemJid,ditem.node);
          else
            FDiscovery->removeDiscoInfo(ditem.itemJid,ditem.node);
        }
      }
    }
  }
  else if (action == FDiscoInfo)
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem)
    {
      Jid itemJid = treeItem->data(0,DDR_JID).toString();
      QString itemNode = treeItem->data(0,DDR_NODE).toString();
      FDiscovery->showDiscoInfo(FStreamJid,itemJid,itemNode,this);
    }
  }
  else if (action == FAddContact)
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem)
    {
      Jid itemJid = treeItem->data(0,DDR_JID).toString();
      QString itemName = treeItem->data(0,DDR_NAME).toString();
      FRosterChanger->showAddContactDialog(FStreamJid,itemJid,itemName,"","");
    }
  }
  else if (action == FShowVCard)
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem)
    {
      Jid itemJid = treeItem->data(0,DDR_JID).toString();
      FVCardPlugin->showVCardDialog(FStreamJid,itemJid);
    }
  }
}

void DiscoItemsWindow::onComboReturnPressed()
{
  Jid itemJid = ui.cmbJid->currentText();
  QString itemNode = ui.cmbNode->currentText();
  if (itemJid.isValid() && FDiscoverySteps.value(FCurrentStep) != qMakePair(itemJid,itemNode))
    discover(itemJid,itemNode);
}

void DiscoItemsWindow::onStreamJidChanged(const Jid &ABefour, const Jid &AAftert)
{
  FStreamJid = AAftert;
  emit streamJidChanged(ABefour,AAftert);
}

