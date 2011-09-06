#include "notifyoptionswidget.h"

#include <QHeaderView>

enum ModelColumns
{
	COL_NAME,
	COL_ENABLE,
	COL_COUNT
};

enum ModelDataRoles
{
	MDR_KIND = Qt::UserRole + 1,
	MDR_SORT
};

bool SortFilterProxyModel::lessThan( const QModelIndex &ALeft, const QModelIndex &ARight ) const
{
	return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

NotifyOptionsWidget::NotifyOptionsWidget(INotifications *ANotifications, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FNotifications = ANotifications;
	FBlockChangesCheck = 0;

	createTreeModel();

	FSortModel.setSourceModel(&FModel);
	FSortModel.setSortLocaleAware(true);
	FSortModel.setSortCaseSensitivity(Qt::CaseInsensitive);
	FSortModel.setSortRole(MDR_SORT);

	ui.trvNotifies->setModel(&FSortModel);
	ui.trvNotifies->header()->hide();
	ui.trvNotifies->header()->setResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.trvNotifies->header()->setResizeMode(COL_ENABLE,QHeaderView::ResizeToContents);
	ui.trvNotifies->sortByColumn(COL_NAME,Qt::AscendingOrder);
	ui.trvNotifies->setItemsExpandable(false);
	ui.trvNotifies->expandAll();

	connect(ui.pbtRestoreDefaults,SIGNAL(clicked()),SLOT(onRestoreDefaultsClicked()));
	connect(&FModel,SIGNAL(itemChanged(QStandardItem *)),SLOT(onModelItemChanged(QStandardItem *)));

	reset();
}

NotifyOptionsWidget::~NotifyOptionsWidget()
{
	connect(ui.spbPopupTimeout,SIGNAL(valueChanged(int)),SIGNAL(modified()));
}

void NotifyOptionsWidget::apply()
{
	Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).setValue(ui.spbPopupTimeout->value());
	for (QMap<int, QStandardItem *>::const_iterator it=FKindItems.constBegin(); it!=FKindItems.constEnd(); it++)
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(it.key())).setValue(it.value()->checkState() == Qt::Checked);

	foreach(QString typeId, FTypeItems.uniqueKeys())
	{
		ushort kinds = FNotifications->notificationKinds(typeId);
		foreach(QStandardItem *typeItem, FTypeItems.values(typeId))
		{
			if (typeItem->checkState() == Qt::Checked)
				kinds |= (ushort)typeItem->data(MDR_KIND).toInt();
			else
				kinds &= ~((ushort)typeItem->data(MDR_KIND).toInt());
		}
		FNotifications->setNotificationKinds(typeId,kinds);
	}

	emit childApply();
}

void NotifyOptionsWidget::reset()
{
	ui.spbPopupTimeout->setValue(Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt());
	
	for (QMap<int, QStandardItem *>::const_iterator it=FKindItems.constBegin(); it!=FKindItems.constEnd(); it++)
		it.value()->setCheckState(Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(it.key())).value().toBool() ? Qt::Checked : Qt::Unchecked);
	
	for (QMultiMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); it++)
		it.value()->setCheckState((FNotifications->notificationKinds(it.key()) & it.value()->data(MDR_KIND).toInt())>0 ? Qt::Checked : Qt::Unchecked);

	void childReset();
}

void NotifyOptionsWidget::createTreeModel()
{
	static const struct { ushort kind; QString name; } KindsList[] = { 
		{ INotification::PopupWindow, tr("Display a notification in popup window") },
		{ INotification::SoundPlay, tr("Play sound at the notification") },
		{ INotification::ShowMinimized, tr("Show the corresponding window minimized in the taskbar") },
		{ INotification::AlertWidget, tr("Highlight the corresponding window in the taskbar") },
		{ INotification::TabPageNotify, tr("Display a notification in tab") },
		{ INotification::RosterNotify, tr("Display a notification in your roster") },
		{ INotification::TrayNotify, tr("Display a notification in tray") },
		{ INotification::TrayAction, tr("Display a notification in tray context menu") },
		{ INotification::AutoActivate, tr("Immediately activate the notification") },
		{ 0, QString::null }
	};

	FModel.clear();
	FModel.setColumnCount(2);

	foreach(QString typeId, FNotifications->notificationTypes())
	{
		INotificationType notifyType = FNotifications->notificationType(typeId);
		if (!notifyType.title.isEmpty() && notifyType.kindMask>0)
		{
			for (int index =0; KindsList[index].kind!=0; index++)
			{
				if ((notifyType.kindMask & KindsList[index].kind)>0)
				{
					QStandardItem *kindEnableItem = FKindItems.value(KindsList[index].kind);
					if (kindEnableItem == NULL)
					{
						QStandardItem *kindNameItem = new QStandardItem(KindsList[index].name);
						kindNameItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
						kindNameItem->setData(index,MDR_SORT);

						kindEnableItem = new QStandardItem();
						kindEnableItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
						kindEnableItem->setTextAlignment(Qt::AlignCenter);
						kindEnableItem->setCheckable(true);
						kindEnableItem->setCheckState(Qt::PartiallyChecked);

						FKindItems.insert(KindsList[index].kind,kindEnableItem);
						FModel.invisibleRootItem()->appendRow(QList<QStandardItem *>() << kindNameItem << kindEnableItem);
					}
					QStandardItem *kindNameItem = FModel.item(kindEnableItem->row(),COL_NAME);

					QStandardItem *typeNameItem = new QStandardItem(notifyType.title);
					typeNameItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
					typeNameItem->setData(notifyType.order,MDR_SORT);
					typeNameItem->setIcon(notifyType.icon);

					QStandardItem *typeEnableItem = new QStandardItem;
					typeEnableItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
					typeEnableItem->setData(KindsList[index].kind,MDR_KIND);
					typeEnableItem->setTextAlignment(Qt::AlignCenter);
					typeEnableItem->setCheckable(true);
					typeEnableItem->setCheckState(Qt::PartiallyChecked);

					FTypeItems.insertMulti(typeId,typeEnableItem);
					kindNameItem->appendRow(QList<QStandardItem *>() << typeNameItem << typeEnableItem);
				}
			}
		}
	}
}

void NotifyOptionsWidget::setItemGray(QStandardItem *AItem, bool AGray) const
{
	AItem->setForeground(ui.trvNotifies->palette().color(AGray ? QPalette::Disabled : QPalette::Normal, QPalette::Text));
}

void NotifyOptionsWidget::setItemBold(QStandardItem *AItem, bool ABold) const
{
	QFont font = AItem->font();
	font.setBold(ABold);
	AItem->setFont(font);
}

void NotifyOptionsWidget::onRestoreDefaultsClicked()
{
	for (QMap<int, QStandardItem *>::const_iterator it=FKindItems.constBegin(); it!=FKindItems.constEnd(); it++)
		it.value()->setCheckState(Qt::Checked);

	for (QMultiMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); it++)
	{
		INotificationType notifyType = FNotifications->notificationType(it.key());
		it.value()->setCheckState((notifyType.kindDefs & it.value()->data(MDR_KIND).toInt())>0 ? Qt::Checked : Qt::Unchecked);
	}
}

void NotifyOptionsWidget::onModelItemChanged(QStandardItem *AItem)
{
	if (FBlockChangesCheck<=0)
	{
		FBlockChangesCheck++;

		if (FKindItems.values().contains(AItem))
		{
			bool enabled = AItem->checkState() == Qt::Checked;
			QStandardItem *kindNameItem = FModel.item(AItem->row(),COL_NAME);
			setItemBold(kindNameItem,!enabled);
			for(int row=0; row<kindNameItem->rowCount(); row++)
			{
				setItemGray(kindNameItem->child(row,COL_NAME),!enabled);
			}
		}
		else if (FTypeItems.values().contains(AItem))
		{
			bool enabled = AItem->checkState() == Qt::Checked;
			INotificationType notifyType = FNotifications->notificationType(FTypeItems.key(AItem));
			QStandardItem *typeNameItem = AItem->parent()->child(AItem->row(),COL_NAME);
			setItemBold(typeNameItem, ((AItem->data(MDR_KIND).toInt() & notifyType.kindDefs)>0)!=enabled);
		}

		emit modified();
		FBlockChangesCheck--;
	}
}
