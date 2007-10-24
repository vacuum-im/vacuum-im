#include "receiverswidget.h"

#include <QHeaderView>
#include <QInputDialog>

ReceiversWidget::ReceiversWidget(IMessenger *AMessenger, const Jid &AStreamJid)
{
  ui.setupUi(this);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;

  FPresence = NULL;
  FStatusIcons = NULL;
  FRoster = NULL;

  setWindowIconText(tr("Receivers"));

  connect(ui.pbtSelectAll,SIGNAL(clicked()),SLOT(onSelectAllClicked()));
  connect(ui.pbtSelectNone,SIGNAL(clicked()),SLOT(onSelectNoneClicked()));
  connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddClicked()));
  connect(ui.pbtUpdate,SIGNAL(clicked()),SLOT(onUpdateClicked()));

  initialize();
}

ReceiversWidget::~ReceiversWidget()
{

}

void ReceiversWidget::setStreamJid(const Jid &AStreamJid)
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  initialize();
  emit streamJidChanged(befour);
}

QString ReceiversWidget::receiverName(const Jid &AReceiver) const
{
  QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
  if (contactItem)
    return contactItem->data(0,RDR_Name).toString();
  return QString();
}

void ReceiversWidget::addReceiversGroup(const QString &AGroup)
{
  QTreeWidgetItem *groupItem = FGroupItems.value(AGroup,NULL);
  if (groupItem)
    groupItem->setCheckState(0,Qt::Checked);
}

void ReceiversWidget::removeReceiversGroup(const QString &AGroup)
{
  QTreeWidgetItem *groupItem = FGroupItems.value(AGroup,NULL);
  if (groupItem)
    groupItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::addReceiver(const Jid &AReceiver)
{
  QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
  if (!contactItem)
  {
    if (AReceiver.isValid())
    {
      QTreeWidgetItem *groupItem = getReceiversGroup(tr("Not in Roster"));
      groupItem->setExpanded(true);
      QString name = AReceiver.node().isEmpty() ? AReceiver.domane() : AReceiver.node();
      contactItem = getReceiver(AReceiver,name,groupItem);
      contactItem->setCheckState(0,Qt::Checked);
    }
  }
  else
    contactItem->setCheckState(0,Qt::Checked);
}

void ReceiversWidget::removeReceiver(const Jid &AReceiver)
{
  QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
  if (contactItem)
    contactItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (presencePlugin)
      FPresence = presencePlugin->getPresence(FStreamJid);
  }

  plugin = FMessenger->pluginManager()->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (rosterPlugin)
      FRoster = rosterPlugin->getRoster(FStreamJid);
  }

  plugin = FMessenger->pluginManager()->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
  }

  if (FRoster && FPresence)
    createRosterTree();
}

QTreeWidgetItem *ReceiversWidget::getReceiversGroup(const QString &AGroup)
{
  QString curGroup;
  QString groupDelim = FRoster->groupDelimiter();
  QTreeWidgetItem *parentGroupItem = ui.trwReceivers->invisibleRootItem();
  QStringList subGroups = AGroup.split(groupDelim,QString::SkipEmptyParts);
  foreach(QString subGroup,subGroups)
  {
    curGroup = curGroup.isEmpty() ? subGroup : curGroup+groupDelim+subGroup;
    QTreeWidgetItem *groupItem = FGroupItems.value(curGroup,NULL);
    if (groupItem == NULL)
    {
      QStringList columns = QStringList() << ' '+subGroup << "";
      groupItem = new QTreeWidgetItem(parentGroupItem,columns);
      groupItem->setCheckState(0,parentGroupItem->checkState(0));
      groupItem->setForeground(0,Qt::blue);
      groupItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
      groupItem->setData(0,RDR_Type,RIT_Group);
      groupItem->setData(0,RDR_Group,curGroup);
      FGroupItems.insert(curGroup,groupItem);
    }
    parentGroupItem = groupItem;
  }
  return parentGroupItem;
}

QTreeWidgetItem *ReceiversWidget::getReceiver(const Jid &AReceiver, const QString &AName, QTreeWidgetItem *AParent)
{
  QTreeWidgetItem *contactItem = NULL;
  QList<QTreeWidgetItem *> contactItems = FContactItems.values(AReceiver);
  for (int i = 0; contactItem == NULL && i<contactItems.count(); i++)
    if (contactItems.at(i)->parent() == AParent)
      contactItem = contactItems.at(i);

  if (!contactItem)
  {
    QStringList columns = QStringList() << AName << AReceiver.full();
    contactItem = new QTreeWidgetItem(AParent,columns);
    contactItem->setIcon(0,FStatusIcons->iconByJid(FStreamJid,AReceiver));
    contactItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    contactItem->setData(0,RDR_Type,RIT_Contact);
    contactItem->setData(0,RDR_Jid,AReceiver.full());
    contactItem->setData(0,RDR_Name,AName);
    FContactItems.insert(AReceiver,contactItem);
  }
  return contactItem;
}

void ReceiversWidget::createRosterTree()
{
  onSelectNoneClicked();
  disconnect(ui.trwReceivers,SIGNAL(itemChanged(QTreeWidgetItem *,int)),this,SLOT(onReceiversItemChanged(QTreeWidgetItem *,int)));
  ui.trwReceivers->clear();
  FGroupItems.clear();
  FContactItems.clear();
  ui.trwReceivers->setColumnCount(2);
  ui.trwReceivers->setIndentation(10);
  ui.trwReceivers->setHeaderLabels(QStringList() << tr("Contact") << tr("Jid"));
  QString groupDelim = FRoster->groupDelimiter();
  QList<IRosterItem *> rosterItems = FRoster->items();
  foreach (IRosterItem *rosterItem, rosterItems)
  {
    QSet<QString> groups = rosterItem->groups();
    if (rosterItem->jid().node().isEmpty())
    {
      groups.clear();
      groups.insert(tr("Agents"));
    }
    else if (groups.isEmpty())
      groups.insert(tr("Blank group"));

    foreach(QString group,groups)
    {
      QTreeWidgetItem *groupItem = getReceiversGroup(group);

      QList<Jid> itemJids;
      QList<IPresenceItem *> presenceItems = FPresence->items(rosterItem->jid());
      foreach(IPresenceItem *presenceItem,presenceItems)
        itemJids.append(presenceItem->jid());
      if (itemJids.isEmpty())
        itemJids.append(rosterItem->jid());
      foreach(Jid itemJid, itemJids)
      {
        QString name = itemJid.resource().isEmpty() ? rosterItem->name() : rosterItem->name()+"/"+itemJid.resource();
        QTreeWidgetItem *contactItem = getReceiver(itemJid,name,groupItem);
        contactItem->setCheckState(0, Qt::Unchecked);
      }
    }
  }

  QList<IPresenceItem *> myResources = FPresence->items(FStreamJid);
  foreach(IPresenceItem *presenceItem, myResources)
  {
    QTreeWidgetItem *groupItem = getReceiversGroup(tr("My Resources"));
    QString name = presenceItem->jid().resource();
    QTreeWidgetItem *contactItem = getReceiver(presenceItem->jid(),name,groupItem);
    contactItem->setCheckState(0, Qt::Unchecked);
  }

  ui.trwReceivers->expandAll();
  ui.trwReceivers->sortItems(0,Qt::AscendingOrder);
  ui.trwReceivers->header()->setResizeMode(0,QHeaderView::ResizeToContents);
  ui.trwReceivers->setSelectionMode(QAbstractItemView::NoSelection);
  connect(ui.trwReceivers,SIGNAL(itemChanged(QTreeWidgetItem *,int)),SLOT(onReceiversItemChanged(QTreeWidgetItem *,int)));
}

void ReceiversWidget::onReceiversItemChanged(QTreeWidgetItem *AItem, int /*AColumn*/)
{
  static int blockUpdateChilds = 0;

  if (AItem->data(0,RDR_Type).toInt() == RIT_Contact)
  {
    Jid contactJid = AItem->data(0,RDR_Jid).toString();
    if (AItem->checkState(0) == Qt::Checked && !FReceivers.contains(contactJid))
    {
      FReceivers.append(contactJid);
      emit receiverAdded(contactJid);
    }
    else if (AItem->checkState(0) == Qt::Unchecked && FReceivers.contains(contactJid))
    {
      FReceivers.removeAt(FReceivers.indexOf(contactJid));
      emit receiverRemoved(contactJid);
    }

    QList<QTreeWidgetItem *> contactItems = FContactItems.values(contactJid);
    foreach(QTreeWidgetItem *contactItem,contactItems)
      contactItem->setCheckState(0,AItem->checkState(0));
  }
  else if (blockUpdateChilds == 0 && AItem->data(0,RDR_Type).toInt() == RIT_Group)
  {
    for (int i =0; i< AItem->childCount(); i++)
      AItem->child(i)->setCheckState(0,AItem->checkState(0));
  }

  if (AItem->parent() != NULL)
  {
    blockUpdateChilds++;
    if (AItem->checkState(0) == Qt::Checked)
    {
      QTreeWidgetItem *parentItem = AItem->parent();
      Qt::CheckState state = Qt::Checked;
      for (int i =0; state == Qt::Checked && i < parentItem->childCount(); i++)
        state = parentItem->child(i)->checkState(0);
      if (state == Qt::Checked)
        parentItem->setCheckState(0,Qt::Checked);
    }
    else
      AItem->parent()->setCheckState(0,Qt::Unchecked);
    blockUpdateChilds--;
  }
}

void ReceiversWidget::onSelectAllClicked()
{
  foreach(QTreeWidgetItem *treeItem,FGroupItems)
    treeItem->setCheckState(0,Qt::Checked);
}

void ReceiversWidget::onSelectNoneClicked()
{
  foreach(QTreeWidgetItem *treeItem,FContactItems)
    treeItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::onAddClicked()
{
  Jid contactJid = QInputDialog::getText(this,tr("Add receiver"),tr("Enter valid contact jid:"));
  if (contactJid.isValid())
    addReceiver(contactJid);
}

void ReceiversWidget::onUpdateClicked()
{
  QList<Jid> savedReceivers = FReceivers;
  createRosterTree();
  foreach(Jid receiver, savedReceivers)
    addReceiver(receiver);
}
