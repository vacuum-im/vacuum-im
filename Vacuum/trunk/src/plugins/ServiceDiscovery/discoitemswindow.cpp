#include "discoitemswindow.h"

#include <QHeaderView>
#include <QLineEdit>

#define CNAME                 0
#define CJID                  1
#define CNODE                 2

#define MAX_ITEMS             20

#define IN_ARROW_LEFT         "psi/arrowLeft"
#define IN_ARROW_RIGHT        "psi/arrowRight"
#define IN_DISCOVER           "psi/jabber"
#define IN_RELOAD             "psi/reload"
#define IN_DISCO              "psi/disco"
#define IN_ADDCONTACT         "psi/addContact"
#define IN_VCARD              "psi/vCard"
#define IN_INFO               "psi/statusmsg"

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
  ui.trwItems->header()->setResizeMode(CNAME,QHeaderView::Stretch);
  ui.trwItems->header()->setResizeMode(CJID,QHeaderView::ResizeToContents);
  ui.trwItems->header()->setResizeMode(CNODE,QHeaderView::ResizeToContents);
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

  if (!FDiscovery->hasDiscoInfo(AContactJid,ANode))
    requestDiscoInfo(AContactJid,ANode);
  else
    updateDiscoInfo(FDiscovery->discoInfo(AContactJid,ANode));

  if (!FDiscovery->hasDiscoItems(AContactJid,ANode))
    requestDiscoItems(AContactJid,ANode);
  else
    updateDiscoItems(FDiscovery->discoItems(AContactJid,ANode));

  treeItem->setExpanded(true);
  ui.trwItems->setCurrentItem(treeItem);
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
  QTreeWidgetItem *treeItem = new QTreeWidgetItem();
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
      toolTip+=tr("<b>Identity:</b>");
      foreach(IDiscoIdentity identity, ADiscoInfo.identity)
        toolTip+=tr("<li>%1 (Category: '%2'; Type: '%3')</li>").arg(identity.name).arg(identity.category).arg(identity.type);

      toolTip+=tr("<br><b>Features:</b>");
      foreach(QString feature, ADiscoInfo.features)
      {
        IDiscoFeature dfeature = FDiscovery->discoFeature(feature);
        toolTip+=QString("<li>%1</li>").arg(dfeature.name.isEmpty() ? feature : dfeature.name);
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
        if (FDiscovery->hasDiscoInfo(ditem.itemJid,ditem.node))
          updateDiscoInfo(FDiscovery->discoInfo(ditem.itemJid,ditem.node));
        else if (ADiscoItems.items.count() <= MAX_ITEMS)
          requestDiscoInfo(ditem.itemJid,ditem.node);
        else
          curItem->setIcon(CNAME,FDiscovery->discoItemIcon(ditem));
        if (FDiscovery->hasDiscoItems(ditem.itemJid,ditem.node))
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
  FToolBarChanger->addAction(FMoveBack,AG_DIWT_SERVICEDISCOVERY_NAVIGATE,false);
  connect(FMoveBack,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FMoveForward = new Action(FToolBarChanger);
  FMoveForward->setText(tr("Forward"));
  FMoveForward->setIcon(SYSTEM_ICONSETFILE,IN_ARROW_RIGHT);
  FToolBarChanger->addAction(FMoveForward,AG_DIWT_SERVICEDISCOVERY_NAVIGATE,false);
  connect(FMoveForward,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FDiscoverCurrent = new Action(FToolBarChanger);
  FDiscoverCurrent->setText(tr("Discover"));
  FDiscoverCurrent->setIcon(SYSTEM_ICONSETFILE,IN_DISCOVER);
  FToolBarChanger->addAction(FDiscoverCurrent,AG_DIWT_SERVICEDISCOVERY_DEFACTIONS,false);
  connect(FDiscoverCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FReloadCurrent = new Action(FToolBarChanger);
  FReloadCurrent->setText(tr("Reload"));
  FReloadCurrent->setIcon(SYSTEM_ICONSETFILE,IN_RELOAD);
  FToolBarChanger->addAction(FReloadCurrent,AG_DIWT_SERVICEDISCOVERY_DEFACTIONS,false);
  connect(FReloadCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FDiscoInfo = new Action(FToolBarChanger);
  FDiscoInfo->setText(tr("Disco info"));
  FDiscoInfo->setIcon(SYSTEM_ICONSETFILE,IN_INFO);
  FToolBarChanger->addAction(FDiscoInfo,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
  connect(FDiscoInfo,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FAddContact = new Action(FToolBarChanger);
  FAddContact->setText(tr("Add Contact"));
  FAddContact->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
  FToolBarChanger->addAction(FAddContact,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
  connect(FAddContact,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FShowVCard = new Action(FToolBarChanger);
  FShowVCard->setText(tr("vCard"));
  FShowVCard->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
  FToolBarChanger->addAction(FShowVCard,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
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
  QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
  if (treeItem)
  {
    QList<Action *> curActions = FActionsBarChanger->actions(AG_DIWT_SERVICEDISCOVERY_FEATURE_ACTIONS);
    IDiscoInfo dinfo = FDiscovery->discoInfo(treeItem->data(0,DDR_JID).toString(),treeItem->data(0,DDR_NODE).toString());
    foreach(QString feature, dinfo.features)
    {
      if (FDiscovery->hasFeatureHandler(feature))
      {
        Action *action = FFeatureActions.value(feature,NULL);
        if (action == NULL)
        {
          action = new Action(this);
          IDiscoFeature dfeature = FDiscovery->discoFeature(feature);
          action->setIcon(dfeature.icon);
          action->setText(dfeature.actionName.isEmpty() ? (dfeature.name.isEmpty() ? feature : dfeature.name) : dfeature.actionName);
          action->setToolTip(dfeature.description);
          connect(action,SIGNAL(triggered()),SLOT(onFeatureActionTriggered()));
          FFeatureActions.insert(feature,action);
        }
        if (!curActions.contains(action))
        {
          FActionsBarChanger->addAction(action,AG_DIWT_SERVICEDISCOVERY_FEATURE_ACTIONS,true);
          emit featureActionInserted(feature,action);
        }
        else
          curActions.removeAt(curActions.indexOf(action));
      }
    }
    foreach(Action *action,curActions)
    {
      FActionsBarChanger->removeAction(action);
      emit featureActionRemoved(FFeatureActions.key(action),action);
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
    ui.trwItems->setCurrentItem(treeItem);
    Menu *menu = new Menu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose,true);

    menu->addAction(FDiscoverCurrent,AG_DIWT_SERVICEDISCOVERY_DEFACTIONS,false);
    menu->addAction(FReloadCurrent,AG_DIWT_SERVICEDISCOVERY_DEFACTIONS,false);
    menu->addAction(FDiscoInfo,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
    menu->addAction(FAddContact,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
    menu->addAction(FShowVCard,AG_DIWT_SERVICEDISCOVERY_ACTIONS,false);
    
    IDiscoInfo dinfo = FDiscovery->discoInfo(treeItem->data(0,DDR_JID).toString(),treeItem->data(0,DDR_NODE).toString());
    foreach(QString feature, dinfo.features)
      if (FFeatureActions.contains(feature))
        menu->addAction(FFeatureActions.value(feature),AG_DIWT_SERVICEDISCOVERY_FEATURE_ACTIONS,false);
    
    emit treeItemContextMenu(treeItem,menu);

    menu->popup(ui.trwItems->viewport()->mapToGlobal(APos));
  }
}

void DiscoItemsWindow::onTreeItemExpanded(QTreeWidgetItem *AItem)
{
  if (AItem->childCount()==0 && !AItem->data(0,DDR_REQUEST_ITEMS).toBool())
  {
    Jid itemJid = AItem->data(0,DDR_JID).toString();
    QString itemNode = AItem->data(0,DDR_NODE).toString();
    if (!FDiscovery->hasDiscoItems(itemJid,itemNode))
      requestDiscoItems(itemJid,itemNode);
    else
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
      if (!FDiscovery->hasDiscoInfo(itemJid,itemNode))
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

void DiscoItemsWindow::onFeatureActionTriggered()
{
  Action *action = qobject_cast<Action *>(sender());
  QString feature = FFeatureActions.key(action);
  if (!feature.isEmpty())
  {
    QTreeWidgetItem *treeItem = ui.trwItems->currentItem();
    if (treeItem)
    {
      IDiscoItem ditem;
      ditem.itemJid = treeItem->data(0,DDR_JID).toString();
      ditem.node = treeItem->data(0,DDR_NODE).toString();
      ditem.name = treeItem->data(0,DDR_NAME).toString();
      FDiscovery->execFeatureHandler(FStreamJid,feature,ditem);
    }
  }
}

void DiscoItemsWindow::onStreamJidChanged(const Jid &ABefour, const Jid &AAftert)
{
  FStreamJid = AAftert;
  emit streamJidChanged(ABefour,AAftert);
}

