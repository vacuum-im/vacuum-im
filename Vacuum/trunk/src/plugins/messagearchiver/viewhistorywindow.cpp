#include "viewhistorywindow.h"

#include <QScrollBar>
#include <QListView>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

#define MINIMUM_DATETIME      QDateTime(QDate(1,1,1),QTime(0,0,0))
#define MAXIMUM_DATETIME      QDateTime::currentDateTime()

#define ADR_GROUP_KIND        Action::DR_Parametr1

#define BIN_SPLITTER_STATE    "ArchiveWindowSplitterState"
#define BIN_WINDOW_GEOMETRY   "ArchiveWindowGeometry"

#define IN_HISTORY            "psi/history"
#define IN_GROUP_KIND         ""
#define IN_FILTER             ""
#define IN_RENAME             ""
#define IN_REMOVE             ""
#define IN_RELOAD             ""


//SortFilterProxyModel
SortFilterProxyModel::SortFilterProxyModel(ViewHistoryWindow *AWindow, QObject *AParent) : QSortFilterProxyModel(AParent)
{
  FWindow = AWindow;
  setDynamicSortFilter(false);
  setSortLocaleAware(true);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SortFilterProxyModel::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
  QModelIndex index = sourceModel()->index(ARow,0,AParent);
  int indexType = index.data(HDR_ITEM_TYPE).toInt();
  if (indexType == HIT_HEADER_JID)
  {
    IArchiveHeader header;
    header.with = index.data(HDR_HEADER_WITH).toString();
    header.start = index.data(HDR_HEADER_START).toDateTime();
    header.threadId = index.data(HDR_HEADER_THREAD).toString();
    header.subject = index.data(HDR_HEADER_SUBJECT).toString();
    return FWindow->isHeaderAccepted(header);
  }
  else if (indexType == HIT_GROUP_CONTACT)
  {
    Jid indexWith = index.data(HDR_HEADER_WITH).toString();
    for (int i=0; i<sourceModel()->rowCount(index);i++)
      if (filterAcceptsRow(i,index))
        return true;
    return false;
  }
  else if (indexType == HIT_GROUP_DATE)
  {
    const IArchiveFilter &filter = FWindow->filter();
    QDateTime indexStart = index.data(HDR_DATE_START).toDateTime();
    QDateTime indexEnd = index.data(HDR_DATE_END).toDateTime();
    if ((!filter.start.isValid() || filter.start<indexEnd) && (!filter.end.isValid() || indexStart < filter.end))
      for (int i=0; i<sourceModel()->rowCount(index);i++)
        if (filterAcceptsRow(i,index))
          return true;
    return false;
  }
  return true;
}

//ViewHistoryWindow
ViewHistoryWindow::ViewHistoryWindow(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent) : QMainWindow(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("View History - %1").arg(AStreamJid.bare()));
  setWindowIcon(Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_HISTORY));

  FRoster = NULL;
  FViewWidget = NULL;
  FSettings = NULL;
  FMessenger = NULL;
  FGroupsTools = NULL;
  FMessagesTools = NULL;

  FArchiver = AArchiver;
  FStreamJid = AStreamJid;
  FGroupKind = GK_CONTACT;

  QToolBar *groupsToolBar = this->addToolBar("Groups Tools");
  FGroupsTools = new ToolBarChanger(groupsToolBar);
  static_cast<QBoxLayout *>(ui.grbGroups->layout())->insertWidget(0,groupsToolBar);

  QListView *cmbView= new QListView(ui.cmbContact);
  QSortFilterProxyModel *cmbSort = new QSortFilterProxyModel(ui.cmbContact);
  cmbSort->setSortCaseSensitivity(Qt::CaseInsensitive);
  cmbSort->setSortLocaleAware(true);
  cmbSort->setSourceModel(ui.cmbContact->model());
  cmbSort->setSortRole(Qt::DisplayRole);
  cmbView->setModel(cmbSort);
  ui.cmbContact->setView(cmbView);

  ui.lneText->setFocus();

  FModel = new QStandardItemModel(0,3,this);
  FModel->setHorizontalHeaderLabels(QStringList() << tr("With") << tr("Date") << tr("Subject"));
  
  FProxyModel = new SortFilterProxyModel(this);
  FProxyModel->setSourceModel(FModel);
  FProxyModel->setSortRole(HDR_SORT_ROLE);

  ui.trvCollections->setModel(FProxyModel);
  ui.trvCollections->header()->setResizeMode(0,QHeaderView::ResizeToContents);
  ui.trvCollections->header()->setResizeMode(1,QHeaderView::ResizeToContents);
  ui.trvCollections->header()->setResizeMode(2,QHeaderView::Stretch);
  ui.trvCollections->sortByColumn(1,Qt::DescendingOrder);
  connect(ui.trvCollections->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));
  connect(ui.trvCollections,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onItemContextMenuRequested(const QPoint &)));

  FInvalidateTimer.setInterval(0);
  FInvalidateTimer.setSingleShot(true);
  connect(&FInvalidateTimer,SIGNAL(timeout()),SLOT(onInvalidateTimeout()));

  connect(FArchiver->instance(),SIGNAL(localCollectionSaved(const Jid &, const IArchiveHeader &)),
    SLOT(onLocalCollectionSaved(const Jid &, const IArchiveHeader &)));
  connect(FArchiver->instance(),SIGNAL(localCollectionRemoved(const Jid &, const IArchiveHeader &)),
    SLOT(onLocalCollectionRemoved(const Jid &, const IArchiveHeader &)));
  connect(FArchiver->instance(),SIGNAL(serverHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const IArchiveResultSet &)),
    SLOT(onServerHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const IArchiveResultSet &)));
  connect(FArchiver->instance(),SIGNAL(serverCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)),
    SLOT(onServerCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)));
  connect(FArchiver->instance(),SIGNAL(serverCollectionSaved(const QString &, const IArchiveHeader &)),
    SLOT(onServerCollectionSaved(const QString &, const IArchiveHeader &)));
  connect(FArchiver->instance(),SIGNAL(serverCollectionsRemoved(const QString &, const IArchiveRequest &)),
    SLOT(onServerCollectionsRemoved(const QString &, const IArchiveRequest &)));
  connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),SLOT(onRequestFailed(const QString &, const QString &)));

  connect(ui.pbtApply,SIGNAL(clicked()),SLOT(onApplyFilterClicked()));
  connect(ui.lneText,SIGNAL(returnPressed()),SLOT(onApplyFilterClicked()));
  connect(ui.cmbContact->lineEdit(),SIGNAL(returnPressed()),SLOT(onApplyFilterClicked()));

  createGroupKindMenu();
  createHeaderActions();
  initialize();
}

ViewHistoryWindow::~ViewHistoryWindow()
{
  if (FSettings)
  {
    FSettings->saveBinaryData(BIN_SPLITTER_STATE+FStreamJid.pBare(),ui.splitter->saveState());
    FSettings->saveBinaryData(BIN_WINDOW_GEOMETRY+FStreamJid.pBare(),saveGeometry());
  }
  clearModel();
  emit windowDestroyed(this);
}

bool ViewHistoryWindow::isHeaderAccepted(const IArchiveHeader &AHeader) const
{
  Jid gateFilter = FArchiver->gateJid(FFilter.with);
  Jid gateWith = FArchiver->gateJid(AHeader.with);
  bool accepted = !gateFilter.isValid() ||  (gateFilter.resource().isEmpty() ? gateFilter && gateWith : gateFilter == gateWith);
  accepted &= (!FFilter.start.isValid() || FFilter.start<=AHeader.start) && (!FFilter.end.isValid() || AHeader.start < FFilter.end);
  accepted &= FFilter.threadId.isEmpty() || FFilter.threadId==AHeader.threadId;
  if (accepted && !FFilter.body.isEmpty() && FFilter.body.indexIn(AHeader.subject) < 0)
  {
    IArchiveCollection collection = FCollections.value(AHeader);
    if (collection.messages.isEmpty() && collection.notes.isEmpty())
      collection = FArchiver->loadLocalCollection(FStreamJid,AHeader);

    accepted = false;
    for (int i=0; !accepted && i<collection.messages.count(); i++)
      accepted = FFilter.body.indexIn(collection.messages.at(i).body())>=0;

    QStringList notes = collection.notes.values();
    for (int i=0; !accepted && i<notes.count(); i++)
      accepted = FFilter.body.indexIn(notes.at(i))>=0;
  }
  return accepted;
}

QStandardItem *ViewHistoryWindow::findHeaderItem(const IArchiveHeader &AHeader, QStandardItem *AParent) const
{
  int rowCount = AParent ? AParent->rowCount() : FModel->rowCount();
  for (int row=0; row<rowCount; row++)
  {
    QStandardItem *item = AParent ? AParent->child(row,0) : FModel->item(row,0);
    if (item->data(HDR_ITEM_TYPE) == HIT_HEADER_JID)
    {
      Jid with = item->data(HDR_HEADER_WITH).toString();
      QDateTime start = item->data(HDR_HEADER_START).toDateTime();
      if (AHeader.with == with && AHeader.start == start)
        return item;
    }
    item = findHeaderItem(AHeader,item);
    if (item)
      return item;
  }
  return NULL;
}

void ViewHistoryWindow::setGroupKind(int AGroupKind)
{
  if (FGroupKind != AGroupKind)
  {
    FGroupKind = AGroupKind;
    rebuildModel();
    emit groupKindChanged(AGroupKind);
  }
}

void ViewHistoryWindow::setFilter(const IArchiveFilter &AFilter)
{
  FFilter = AFilter;
  processRequests(createRequests(AFilter));
  updateFilterWidgets();
  FInvalidateTimer.start();
  emit filterChanged(AFilter);
}

void ViewHistoryWindow::reload()
{
  clearModel();
  FHeaderRequests.clear();
  FCollectionRequests.clear();
  FRenameRequests.clear();
  FRemoveRequests.clear();
  FRequestList.clear();
  FCollections.clear();
  processRequests(createRequests(FFilter));
  FInvalidateTimer.start();
}

void ViewHistoryWindow::initialize()
{
  IPluginManager *manager = FArchiver->pluginManager();
  
  IPlugin *plugin = manager->getPlugins("IRosterPlugin").value(0);
  if (plugin)
    FRoster = qobject_cast<IRosterPlugin *>(plugin->instance())->getRoster(FStreamJid);

  plugin = manager->getPlugins("IMessenger").value(0);
  if (plugin)
  {
    FMessenger = qobject_cast<IMessenger *>(plugin->instance());
    if (FMessenger)
    {
      FViewWidget = FMessenger->newViewWidget(FStreamJid,FStreamJid);
      FViewWidget->setColorForJid(FStreamJid.bare(),Qt::red);
      FMessagesTools = FMessenger->newToolBarWidget(NULL,FViewWidget,NULL,NULL);
      QVBoxLayout *layout = new QVBoxLayout(ui.grbMessages);
      layout->setMargin(3);
      layout->addWidget(FMessagesTools);
      layout->addWidget(FViewWidget);
      ui.grbMessages->setLayout(layout);
    }
  }

  plugin = manager->getPlugins("ISettingsPlugin").value(0);
  if (plugin)
  {
    FSettings = qobject_cast<ISettingsPlugin *>(plugin->instance())->settingsForPlugin(MESSAGEARCHIVER_UUID);
    if (FSettings)
    {
      restoreGeometry(FSettings->loadBinaryData(BIN_WINDOW_GEOMETRY+FStreamJid.pBare()));
      ui.splitter->restoreState(FSettings->loadBinaryData(BIN_SPLITTER_STATE+FStreamJid.pBare()));
    }
  }
}

QList<IArchiveRequest> ViewHistoryWindow::createRequests(const IArchiveFilter &AFilter) const
{
  QList<IArchiveRequest> requests;
  
  IArchiveRequest request;
  request.with = AFilter.with;
  request.start = AFilter.start.isValid() ? AFilter.start : MINIMUM_DATETIME;
  request.end = AFilter.end.isValid() ? AFilter.end : MAXIMUM_DATETIME;
  requests.append(request);

  foreach(IArchiveRequest oldReq, FRequestList)
  {
    if (!oldReq.with.isValid() || oldReq.with==AFilter.with)
    {
      for(int i=0; i<requests.count(); i++)
      {
        IArchiveRequest &newReq = requests[i];
        if (newReq.start < oldReq.start && oldReq.end < newReq.end)
        {
          IArchiveRequest newReq2 = newReq;
          newReq2.start = oldReq.end;
          requests.append(newReq2);
          newReq.end = oldReq.start;
        }
        else if (oldReq.start <= newReq.start && newReq.end <= oldReq.end)
        {
          requests.removeAt(i--);
        }
        else if (oldReq.start <= newReq.start && oldReq.end < newReq.end && newReq.start < oldReq.end)
        {
          newReq.start = oldReq.end;
        }
        else if (newReq.start < oldReq.start && oldReq.start < newReq.end && newReq.end <= oldReq.end)
        {
          newReq.end = oldReq.start;
        }
      }
    }
  }
  return requests;
}

void ViewHistoryWindow::divideRequests(const QList<IArchiveRequest> &ARequests, QList<IArchiveRequest> &ALocal, 
                                       QList<IArchiveRequest> &AServer) const
{
  QDateTime replPoint = FArchiver->replicationPoint(FStreamJid);
  foreach (IArchiveRequest request, ARequests)
  {
    if (!replPoint.isValid() || request.end <= replPoint)
    {
      ALocal.append(request);
    }
    else if (replPoint <= request.start)
    {
      AServer.append(request);
    }
    else
    {
      IArchiveRequest serverRequest = request;
      IArchiveRequest localRequest = request;
      serverRequest.start = replPoint;
      localRequest.end = replPoint;
      AServer.append(serverRequest);
      ALocal.append(localRequest);
    }
  }
}

bool ViewHistoryWindow::loadServerHeaders(const IArchiveRequest &ARequest, const QString &AAfter)
{
  QString requestId = FArchiver->loadServerHeaders(FStreamJid,ARequest,AAfter);
  if (!requestId.isEmpty())
  {
    FHeaderRequests.insert(requestId,ARequest);
    return true;
  }
  return false;
}

bool ViewHistoryWindow::loadServerCollection(const IArchiveHeader &AHeader, const QString &AAfter)
{
  QString requestId = FArchiver->loadServerCollection(FStreamJid,AHeader,AAfter);
  if (!requestId.isEmpty())
  {
    FCollectionRequests.insert(requestId,AHeader);
    return true;
  }
  return false;
}

QStandardItem *ViewHistoryWindow::findChildItem(int ARole, const QVariant &AValue, QStandardItem *AParent) const
{
  int rowCount = AParent!=NULL ? AParent->rowCount() : FModel->rowCount();
  for (int i=0; i<rowCount; i++)
  {
    QStandardItem *childItem = AParent!=NULL ? AParent->child(i,0) : FModel->item(i,0);
    if (childItem->data(ARole)==AValue)
      return childItem;
  }
  return NULL;
}

QStandardItem *ViewHistoryWindow::createSortItem(QStandardItem *AOwner)
{
  QStandardItem *item = new QStandardItem;
  item->setData(HIT_SORT_ITEM, HDR_ITEM_TYPE);
  item->setData(AOwner->data(HDR_SORT_ROLE),HDR_SORT_ROLE);
  return item;
}

QStandardItem *ViewHistoryWindow::createCustomItem(int AType, const QVariant &ADiaplay)
{
  QStandardItem *item = new QStandardItem;
  item->setData(AType, HDR_ITEM_TYPE);
  item->setData(FStreamJid.full(),HDR_STREAM_JID);
  item->setData(ADiaplay, HDR_SORT_ROLE);
  item->setData(ADiaplay, Qt::DisplayRole);
  return item;
}

QStandardItem *ViewHistoryWindow::createDateGroup(const IArchiveHeader &AHeader, QStandardItem *AParent)
{
  QDateTime year(QDate(AHeader.start.date().year(),1,1),QTime(0,0,0));
  QStandardItem *yearItem = findChildItem(HDR_DATE_START,year,AParent);
  if (!yearItem)
  {
    yearItem = createCustomItem(HIT_GROUP_DATE,year.date().year());
    yearItem->setData(year,HDR_DATE_START);
    yearItem->setData(year.addYears(1).addSecs(-1),HDR_DATE_END);
    yearItem->setData(year,HDR_SORT_ROLE);
    QList<QStandardItem *> items = QList<QStandardItem *>() << yearItem << createSortItem(yearItem) << createSortItem(yearItem);
    AParent!=NULL ? AParent->appendRow(items) : FModel->appendRow(items);
    emit itemCreated(yearItem);
  }

  QDateTime month(QDate(year.date().year(),AHeader.start.date().month(),1),QTime(0,0,0));
  QStandardItem *monthItem = findChildItem(HDR_DATE_START,month,yearItem);
  if (!monthItem)
  {
    monthItem = createCustomItem(HIT_GROUP_DATE,QDate::longMonthName(month.date().month()));
    monthItem->setData(month,HDR_DATE_START);
    monthItem->setData(month.addMonths(1).addSecs(-1),HDR_DATE_END);
    monthItem->setData(month,HDR_SORT_ROLE);
    yearItem->appendRow(QList<QStandardItem *>() << monthItem << createSortItem(monthItem) << createSortItem(monthItem));
    emit itemCreated(monthItem);
  }

  QDateTime day(QDate(year.date().year(),month.date().month(),AHeader.start.date().day()),QTime(0,0,0));
  QStandardItem *dayItem = findChildItem(HDR_DATE_START,day,monthItem);
  if (!dayItem)
  {
    dayItem = createCustomItem(HIT_GROUP_DATE,day.date());
    dayItem->setData(day,HDR_DATE_START);
    dayItem->setData(day.addDays(1).addSecs(-1),HDR_DATE_END);
    dayItem->setData(day,HDR_SORT_ROLE);
    monthItem->appendRow(QList<QStandardItem *>() << dayItem << createSortItem(dayItem) << createSortItem(dayItem));
    emit itemCreated(dayItem);
  }

  return dayItem;
}

QStandardItem *ViewHistoryWindow::createContactGroup(const IArchiveHeader &AHeader, QStandardItem *AParent)
{
  Jid gateWith = FArchiver->gateJid(AHeader.with);
  QStandardItem *contactItem = findChildItem(HDR_HEADER_WITH,gateWith.prepared().eBare(),AParent);
  if (!contactItem)
  {
    IRosterItem rItem = FRoster->rosterItem(AHeader.with);
    QString name = rItem.name.isEmpty() ? AHeader.with.bare() : rItem.name;
    contactItem = createCustomItem(HIT_GROUP_CONTACT,name);
    contactItem->setData(gateWith.prepared().eBare(),HDR_HEADER_WITH);
    contactItem->setToolTip(AHeader.with.bare());
    QList<QStandardItem *> items = QList<QStandardItem *>() << contactItem << createSortItem(contactItem) << createSortItem(contactItem);
    AParent!=NULL ? AParent->appendRow(items) : FModel->appendRow(items);
    emit itemCreated(contactItem);
  }
  return contactItem;
}

QStandardItem *ViewHistoryWindow::createHeaderParent(const IArchiveHeader &AHeader, QStandardItem *AParent)
{
  if (AParent == NULL)
  {
    switch(FGroupKind)
    {
    case GK_DATE :
    case GK_DATE_CONTACT :
      return createHeaderParent(AHeader,createDateGroup(AHeader,AParent));
    case GK_CONTACT :
    case GK_CONTACT_DATE : 
      return createHeaderParent(AHeader,createContactGroup(AHeader,AParent));
    }
  }
  else if (AParent->data(HDR_ITEM_TYPE).toInt() == HIT_GROUP_CONTACT)
  {
    switch(FGroupKind)
    {
    case GK_CONTACT_DATE : 
      return createHeaderParent(AHeader,createDateGroup(AHeader,AParent));
    }
  }
  else if (AParent->data(HDR_ITEM_TYPE).toInt() == HIT_GROUP_DATE)
  {
    switch(FGroupKind)
    {
    case GK_DATE_CONTACT : 
      return createHeaderParent(AHeader,createContactGroup(AHeader,AParent));
    }
  }
  return AParent;
}

QStandardItem *ViewHistoryWindow::createHeaderItem(const IArchiveHeader &AHeader)
{
  QStandardItem *parentItem = createHeaderParent(AHeader,NULL);
  
  QString name;
  IRosterItem rItem = FRoster->rosterItem(AHeader.with);
  if (!rItem.name.isEmpty())
  {
    name = rItem.name;
    name += !AHeader.with.resource().isEmpty() ? "/"+AHeader.with.resource() : "";
  }
  else
  {
    name = AHeader.with.full();
  }
  QStandardItem *itemJid = createCustomItem(HIT_HEADER_JID,name);
  itemJid->setData(AHeader.with.prepared().eFull(), HDR_HEADER_WITH);
  itemJid->setData(AHeader.start,                   HDR_HEADER_START);
  itemJid->setData(AHeader.subject,                 HDR_HEADER_SUBJECT);
  itemJid->setData(AHeader.threadId,                HDR_HEADER_THREAD);
  itemJid->setData(AHeader.version,                 HDR_HEADER_VERSION);
  itemJid->setToolTip(AHeader.with.full());

  QStandardItem *itemDate = createCustomItem(HIT_HEADER_DATE,AHeader.start);
  QStandardItem *itemSubject = createCustomItem(HIT_HEADER_SUBJECT,AHeader.subject);
  itemSubject->setToolTip(AHeader.subject);

  if (parentItem)
    parentItem->appendRow(QList<QStandardItem *>() << itemJid << itemDate << itemSubject);
  else
    FModel->appendRow(QList<QStandardItem *>() << itemJid << itemDate << itemSubject);
  emit itemCreated(itemJid);

  FInvalidateTimer.start();
  return itemJid;
}

void ViewHistoryWindow::updateHeaderItem(const IArchiveHeader &AHeader)
{
  QStandardItem *headerItem = findHeaderItem(AHeader);
  if (headerItem)
  {
    headerItem->setData(AHeader.subject,HDR_HEADER_SUBJECT);
    headerItem->setData(AHeader.threadId,HDR_HEADER_THREAD);
    headerItem->setData(AHeader.version,HDR_HEADER_VERSION);
    if (headerItem->parent())
      headerItem->parent()->child(headerItem->row(),2)->setText(AHeader.subject);
    else
      FModel->item(headerItem->row(),2)->setText(AHeader.subject);
    IArchiveCollection collection = FCollections.take(AHeader);
    collection.header = AHeader;
    FCollections.insert(AHeader,collection);
  }
}

void ViewHistoryWindow::removeCustomItem(QStandardItem *AItem)
{
  if (AItem)
  {
    while (AItem->rowCount()>0)
      removeCustomItem(AItem->child(0,0));
    emit itemDestroyed(AItem);
    AItem->parent()!=NULL ? AItem->parent()->removeRow(AItem->row()) : qDeleteAll(FModel->takeRow(AItem->row()));
    FInvalidateTimer.start();
  }
}

void ViewHistoryWindow::processRequests(const QList<IArchiveRequest> &ARequests)
{
  QList<IArchiveRequest> localRequests;
  QList<IArchiveRequest> serverRequests;
  divideRequests(ARequests,localRequests,serverRequests);

  foreach(IArchiveRequest request, serverRequests)
  {
    loadServerHeaders(request);
  }

  foreach(IArchiveRequest request, localRequests)
  {
    FRequestList.append(request);
    processHeaders(FArchiver->loadLocalHeaders(FStreamJid,request));
  }
}

void ViewHistoryWindow::processHeaders(const QList<IArchiveHeader> &AHeaders)
{
  foreach(IArchiveHeader header, AHeaders)
  {
    if (!FCollections.contains(header))
    {
      IArchiveCollection collection;
      collection.header = header;
      FCollections.insert(header,collection);
      createHeaderItem(header);
      insertFilterWith(header);
    }
  }
}

void ViewHistoryWindow::processCollection(const IArchiveCollection &ACollection, bool AAppend)
{
  if (FViewWidget && ACollection.header==FCurrentHeader)
  {
    if (!AAppend)
    {
      bool isGroupchat = false;
      for (int i=0; !isGroupchat && i<ACollection.messages.count();i++)
        isGroupchat = ACollection.messages.at(i).type()==Message::GroupChat;

      FViewWidget->textBrowser()->clear();
      FViewWidget->setContactJid(ACollection.header.with);
      if (isGroupchat)
      {
        FViewWidget->setShowKind(IViewWidget::GroupChatMessage);
        FViewWidget->setNickForJid(ACollection.header.with,ACollection.header.with.node());
      }
      else
      {
        FViewWidget->setShowKind(IViewWidget::ChatMessage);
        FViewWidget->setNickForJid(ACollection.header.with, FRoster!=NULL ? FRoster->rosterItem(ACollection.header.with).name : QString());
      }
    }

    QMultiMap<QDateTime,QString>::const_iterator it = ACollection.notes.constBegin();
    while (it!=ACollection.notes.constEnd())
    {
      FViewWidget->showCustomMessage(it.value(),it.key());
      it++;
    }

    foreach(Message message, ACollection.messages)
      FViewWidget->showMessage(message);

    FViewWidget->textBrowser()->verticalScrollBar()->setSliderPosition(0);
  }
}

void ViewHistoryWindow::insertFilterWith(const IArchiveHeader &AHeader)
{
  Jid gateWith = FArchiver->gateJid(AHeader.with);
  IRosterItem rItem = FRoster!=NULL ? FRoster->rosterItem(AHeader.with) : IRosterItem();
  QString name = !rItem.name.isEmpty() ? rItem.name + " ("+gateWith.bare()+")" : gateWith.bare();
  int index = ui.cmbContact->findData(gateWith.pBare());
  if (index < 0)
  {
    ui.cmbContact->addItem(name, gateWith.pBare());
    updateFilterWidgets();
  }
  else if (!rItem.name.isEmpty())
    ui.cmbContact->setItemText(index,name);
  ui.cmbContact->model()->sort(0);
}

void ViewHistoryWindow::updateFilterWidgets()
{
  int index = ui.cmbContact->findData(FFilter.with.pFull());
  index>=0 ? ui.cmbContact->setCurrentIndex(index) : ui.cmbContact->setEditText(FFilter.with.full());
  ui.dedStart->setDate(FFilter.start.isValid() ? FFilter.start.date() : MINIMUM_DATETIME.date());
  ui.dedEnd->setDate(FFilter.end.isValid() ? FFilter.end.date() : MAXIMUM_DATETIME.date());
  ui.lneText->setText(FFilter.body.pattern());
}

void ViewHistoryWindow::clearModel()
{
  while (FModel->rowCount()>0)
    removeCustomItem(FModel->item(0,0));
}

void ViewHistoryWindow::rebuildModel()
{
  clearModel();
  foreach(IArchiveHeader header, FCollections.keys()) {
    createHeaderItem(header); }
}

void ViewHistoryWindow::createGroupKindMenu()
{
  FGroupKindMenu = new Menu(this);
  FGroupKindMenu->setIcon(SYSTEM_ICONSETFILE,IN_GROUP_KIND);
  FGroupKindMenu->setTitle(tr("Groups"));

  Action *action = new Action(FGroupKindMenu);
  action->setText(tr("No groups"));
  action->setData(ADR_GROUP_KIND,GK_NO_GROUPS);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeGroupKindByAction(bool)));
  FGroupKindMenu->addAction(action);

  action = new Action(FGroupKindMenu);
  action->setText(tr("Date"));
  action->setData(ADR_GROUP_KIND,GK_DATE);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeGroupKindByAction(bool)));
  FGroupKindMenu->addAction(action);

  action = new Action(FGroupKindMenu);
  action->setText(tr("Contact"));
  action->setData(ADR_GROUP_KIND,GK_CONTACT);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeGroupKindByAction(bool)));
  FGroupKindMenu->addAction(action);

  action = new Action(FGroupKindMenu);
  action->setText(tr("Date and Contact"));
  action->setData(ADR_GROUP_KIND,GK_DATE_CONTACT);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeGroupKindByAction(bool)));
  FGroupKindMenu->addAction(action);
  
  action = new Action(FGroupKindMenu);
  action->setText(tr("Contact and Date"));
  action->setData(ADR_GROUP_KIND,GK_CONTACT_DATE);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeGroupKindByAction(bool)));
  FGroupKindMenu->addAction(action);

  action = new Action(FGroupKindMenu);
  action->setText(tr("Expand All"));
  connect(action,SIGNAL(triggered()),ui.trvCollections,SLOT(expandAll()));
  FGroupKindMenu->addAction(action,AG_DEFAULT+100);

  action = new Action(FGroupKindMenu);
  action->setText(tr("Collapse All"));
  connect(action,SIGNAL(triggered()),ui.trvCollections,SLOT(collapseAll()));
  FGroupKindMenu->addAction(action,AG_DEFAULT+100);

  QToolButton *button = FGroupsTools->addToolButton(FGroupKindMenu->menuAction(),AG_AWGT_ARCHIVE_GROUPING,false);
  button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  button->setPopupMode(QToolButton::InstantPopup);
}

void ViewHistoryWindow::createHeaderActions()
{
  FFilterBy = new Action(FGroupsTools->toolBar());
  FFilterBy->setText(tr("Filter"));
  FFilterBy->setIcon(SYSTEM_ICONSETFILE,IN_FILTER);
  FFilterBy->setEnabled(false);
  connect(FFilterBy,SIGNAL(triggered(bool)),SLOT(onHeaderActionTriggered(bool)));
  FGroupsTools->addAction(FFilterBy,AG_AWGT_ARCHIVE_DEFACTIONS,false);

  FRename = new Action(FGroupsTools->toolBar());
  FRename->setText(tr("Rename"));
  FRename->setIcon(SYSTEM_ICONSETFILE,IN_RENAME);
  FRename->setEnabled(false);
  connect(FRename,SIGNAL(triggered(bool)),SLOT(onHeaderActionTriggered(bool)));
  FGroupsTools->addAction(FRename,AG_AWGT_ARCHIVE_DEFACTIONS,false);

  FRemove = new Action(FGroupsTools->toolBar());
  FRemove->setText(tr("Remove"));
  FRemove->setIcon(SYSTEM_ICONSETFILE,IN_REMOVE);
  FRemove->setEnabled(false);
  connect(FRemove,SIGNAL(triggered(bool)),SLOT(onHeaderActionTriggered(bool)));
  FGroupsTools->addAction(FRemove,AG_AWGT_ARCHIVE_DEFACTIONS,false);

  FReload = new Action(FGroupsTools->toolBar());
  FReload->setText(tr("Reload"));
  FReload->setIcon(SYSTEM_ICONSETFILE,IN_RELOAD);
  connect(FReload,SIGNAL(triggered(bool)),SLOT(onHeaderActionTriggered(bool)));
  FGroupsTools->addAction(FReload,AG_AWGT_ARCHIVE_DEFACTIONS,false);
}

void ViewHistoryWindow::updateHeaderActions()
{
  bool isHeader = FCurrentHeader.with.isValid() && FCurrentHeader.start.isValid();
  FFilterBy->setEnabled(isHeader && !FFilter.with.isValid());
  FRename->setEnabled(isHeader);
  FRemove->setEnabled(isHeader);
}

void ViewHistoryWindow::onLocalCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
  if (AStreamJid == FStreamJid)
  {
    if (FCollections.contains(AHeader))
      updateHeaderItem(AHeader);
    else
      processHeaders(QList<IArchiveHeader>() << AHeader);
  }
}

void ViewHistoryWindow::onLocalCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
  if (AStreamJid == FStreamJid)
  {
    FCollections.remove(AHeader);
    removeCustomItem(findHeaderItem(AHeader));
  }
}

void ViewHistoryWindow::onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult)
{
  if (FHeaderRequests.contains(AId))
  {
    IArchiveRequest request = FHeaderRequests.take(AId);
    if (AResult.index == 0)
      FRequestList.append(request);
    if (!AResult.last.isEmpty() && AResult.index+AHeaders.count()<AResult.count)
      loadServerHeaders(request,AResult.last);
    processHeaders(AHeaders);
  }
}

void ViewHistoryWindow::onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult)
{
  if (FCollectionRequests.contains(AId))
  {
    IArchiveCollection &collection = FCollections[ACollection.header];
    collection.messages+=ACollection.messages;
    collection.notes+=ACollection.notes;

    if (ACollection.header == FCurrentHeader)
      processCollection(ACollection,AResult.index>0);

    if (!AResult.last.isEmpty() && AResult.index+ACollection.messages.count()+ACollection.notes.count()<AResult.count)
      loadServerCollection(ACollection.header,AResult.last);

    FCollectionRequests.remove(AId);
  }
}

void ViewHistoryWindow::onServerCollectionSaved(const QString &AId, const IArchiveHeader &AHeader)
{
  if (FRenameRequests.contains(AId))
  {
    updateHeaderItem(AHeader);
    FRenameRequests.remove(AId);
  }
}

void ViewHistoryWindow::onServerCollectionsRemoved(const QString &AId, const IArchiveRequest &/*ARequest*/)
{
  if (FRemoveRequests.contains(AId))
  {
    IArchiveHeader header = FRemoveRequests.take(AId);
    QStandardItem *item = findHeaderItem(header);
    if (item)
      removeCustomItem(item);
    FCollections.remove(header);
  }
}

void ViewHistoryWindow::onRequestFailed(const QString &AId, const QString &AError)
{
  if (FHeaderRequests.contains(AId))
  {
    FHeaderRequests.remove(AId);
  }
  else if (FCollectionRequests.contains(AId))
  {
    IArchiveHeader header = FCollectionRequests.take(AId);

    IArchiveCollection &collection = FCollections[header];
    collection.messages.clear();
    collection.notes.clear();

    if (FCurrentHeader == header)
      FViewWidget->showCustomMessage(tr("Message loading failed: %1").arg(AError));
  }
  else if (FRenameRequests.contains(AId))
  {
    FRenameRequests.remove(AId);
  }
  else if (FRemoveRequests.contains(AId))
  {
    FRemoveRequests.remove(AId);
  }
}

void ViewHistoryWindow::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &ABefour)
{
  if (FViewWidget)
  {
    QModelIndex index = ACurrent.column()==0 ? ACurrent : ui.trvCollections->model()->index(ACurrent.row(),0,ACurrent.parent());
    int indexType = index.data(HDR_ITEM_TYPE).toInt();
    if (indexType == HIT_HEADER_JID)
    {
      IArchiveHeader header;
      header.with = index.data(HDR_HEADER_WITH).toString();
      header.start = index.data(HDR_HEADER_START).toDateTime();
      if (FCurrentHeader != header)
      {
        FViewWidget->textBrowser()->clear();
        IArchiveCollection collection = FCollections.value(header);
        FCurrentHeader = collection.header;
        if (collection.messages.isEmpty() && collection.notes.isEmpty())
        {
          if (FArchiver->hasLocalCollection(FStreamJid,FCurrentHeader))
          {
            collection = FArchiver->loadLocalCollection(FStreamJid,FCurrentHeader);
            FCollections.insert(FCurrentHeader,collection);
            processCollection(collection);
          }
          else if (FCollectionRequests.key(FCurrentHeader).isEmpty())
          {
            if (loadServerCollection(FCurrentHeader))
              FViewWidget->showCustomMessage(tr("Loading messages from server ..."));
            else
              FViewWidget->showCustomMessage(tr("Messages request failed."));
          }
        }
        else
          processCollection(collection);
      }
    }
    else
    {
      FCurrentHeader = IArchiveHeader();
      FViewWidget->textBrowser()->clear();
    }
  }
  updateHeaderActions();
  QStandardItem *current = FModel->itemFromIndex(FProxyModel->mapToSource(ACurrent));
  QStandardItem *befour = FModel->itemFromIndex(FProxyModel->mapToSource(ABefour));
  emit itemChanged(current,befour);
}

void ViewHistoryWindow::onItemContextMenuRequested(const QPoint &APos)
{
  QStandardItem *item =  FModel->itemFromIndex(FProxyModel->mapToSource(ui.trvCollections->indexAt(APos)));
  if (item)
  {
    Menu *menu = new Menu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose,true);

    if (FFilterBy->isEnabled())
      menu->addAction(FFilterBy,AG_AWGT_ARCHIVE_DEFACTIONS,false);
    if (FRename->isEnabled())
      menu->addAction(FRename,AG_AWGT_ARCHIVE_DEFACTIONS,false);
    if (FRemove->isEnabled())
      menu->addAction(FRemove,AG_AWGT_ARCHIVE_DEFACTIONS,false);

    emit itemContextMenu(item,menu);
    menu->isEmpty() ? delete menu : menu->popup(ui.trvCollections->viewport()->mapToGlobal(APos));
  }
}

void ViewHistoryWindow::onApplyFilterClicked()
{
  if (ui.dedStart->date() <= ui.dedEnd->date())
  {
    IArchiveFilter filter = FFilter;
    int index = ui.cmbContact->currentIndex();
    if (index>=0 && ui.cmbContact->itemText(index)==ui.cmbContact->currentText())
      filter.with = ui.cmbContact->itemData(ui.cmbContact->currentIndex()).toString();
    else
      filter.with = ui.cmbContact->currentText();
    filter.start = ui.dedStart->dateTime();
    filter.end = ui.dedEnd->dateTime();
    filter.body.setPattern(ui.lneText->text());
    filter.body.setCaseSensitivity(Qt::CaseInsensitive);
    setFilter(filter);
  }
  else
    ui.dedEnd->setDate(ui.dedStart->date());
}

void ViewHistoryWindow::onInvalidateTimeout()
{
  FProxyModel->invalidate();
  QModelIndex index = ui.trvCollections->selectionModel()->currentIndex();
  onCurrentItemChanged(index,index);
}

void ViewHistoryWindow::onChangeGroupKindByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    int groupKind = action->data(ADR_GROUP_KIND).toInt();
    setGroupKind(groupKind);
  }
}

void ViewHistoryWindow::onHeaderActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action == FFilterBy)
  {
    IArchiveFilter filter = FFilter;
    filter.with = FCurrentHeader.with;
    setFilter(filter);
  }
  else if (action == FRename)
  {
    bool ok;
    QString subject = QInputDialog::getText(this,tr("Enter new collection subject"),tr("Subject:"),QLineEdit::Normal,FCurrentHeader.subject,&ok);
    if (ok)
    {
      if (FArchiver->isSupported(FStreamJid))
      {
        IArchiveCollection collection;
        collection.header = FCurrentHeader;
        collection.header.subject = subject;
        QString requestId = FArchiver->saveServerCollection(FStreamJid,collection);
        if (!requestId.isEmpty())
          FRenameRequests.insert(requestId,collection.header);
      }
      if (FArchiver->hasLocalCollection(FStreamJid,FCurrentHeader))
      {
        IArchiveCollection collection = FArchiver->loadLocalCollection(FStreamJid,FCurrentHeader);
        collection.header.subject = subject;
        collection.header.version++;
        FArchiver->saveLocalCollection(FStreamJid,collection,false);
      }
    }
  }
  else if (action == FRemove)
  {
    if (QMessageBox::question(this,tr("Remove collection"), tr("Do you really want to remove this collection with messages?"),
      QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
    {
      if (FArchiver->isSupported(FStreamJid))
      {
        IArchiveRequest request;
        request.with = FCurrentHeader.with;
        request.start = FCurrentHeader.start;
        if (request.with.isValid() && request.start.isValid())
        {
          QString requestId = FArchiver->removeServerCollections(FStreamJid,request);
          if (!requestId.isEmpty())
            FRemoveRequests.insert(requestId,FCurrentHeader);
        }
      }
      FArchiver->removeLocalCollection(FStreamJid,FCurrentHeader);
    }
  }
  else if (action == FReload)
  {
    reload();
  }
}

