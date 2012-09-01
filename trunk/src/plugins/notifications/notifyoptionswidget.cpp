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
	MDR_TYPE = Qt::UserRole + 1,
	MDR_KIND,
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

	connect(ui.spbPopupTimeout,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.pbtRestoreDefaults,SIGNAL(clicked()),SLOT(onRestoreDefaultsClicked()));
	connect(&FModel,SIGNAL(itemChanged(QStandardItem *)),SLOT(onModelItemChanged(QStandardItem *)));

	reset();
}

NotifyOptionsWidget::~NotifyOptionsWidget()
{

}

void NotifyOptionsWidget::apply()
{
	Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).setValue(ui.spbPopupTimeout->value());

	for(QMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); ++it)
	{
		QStandardItem *typeNameItem = it.value();
		ushort kinds = !it.key().isEmpty() ? FNotifications->typeNotificationKinds(it.key()) : 0;
		for (int row=0; row<typeNameItem->rowCount(); row++)
		{
			QStandardItem *kindEnableItem = typeNameItem->child(row, COL_ENABLE);
			if (kindEnableItem->checkState() == Qt::Checked)
				kinds |= (ushort)kindEnableItem->data(MDR_KIND).toInt();
			else
				kinds &= ~((ushort)kindEnableItem->data(MDR_KIND).toInt());
		}

		if (!it.key().isEmpty())
			FNotifications->setTypeNotificationKinds(it.key(),kinds);
		else
			FNotifications->setEnabledNotificationKinds(kinds);
	}

	emit childApply();
}

void NotifyOptionsWidget::reset()
{
	ui.spbPopupTimeout->setValue(Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt());
	for(QMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); ++it)
	{
		QStandardItem *typeNameItem = it.value();
		ushort kinds = !it.key().isEmpty() ? FNotifications->typeNotificationKinds(it.key()) : FNotifications->enabledNotificationKinds();
		for (int row=0; row<typeNameItem->rowCount(); row++)
		{
			QStandardItem *kindEnableItem = typeNameItem->child(row, COL_ENABLE);
			kindEnableItem->setCheckState((kinds & (ushort)kindEnableItem->data(MDR_KIND).toInt())>0 ? Qt::Checked : Qt::Unchecked);
		}
	}
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

	INotificationType globalType;
	globalType.order = 0;
	globalType.kindMask = 0xFFFF;
	globalType.title = tr("Allowed types of notifications");
	globalType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NOTIFICATIONS);

	QMap<QString,INotificationType> notifyTypes;
	notifyTypes.insert(QString::null,globalType);
	foreach(QString typeId, FNotifications->notificationTypes())
		notifyTypes.insert(typeId,FNotifications->notificationType(typeId));

	for(QMap<QString,INotificationType>::const_iterator it=notifyTypes.constBegin(); it!=notifyTypes.constEnd(); ++it)
	{
		if (!it->title.isEmpty() && it->kindMask>0)
		{
			QStandardItem *typeNameItem = new QStandardItem(it->title);
			typeNameItem->setFlags(Qt::ItemIsEnabled);
			typeNameItem->setData(it.key(),MDR_TYPE);
			typeNameItem->setData(it->order,MDR_SORT);
			typeNameItem->setIcon(it->icon);
			setItemBold(typeNameItem,true);

			QStandardItem *typeEnableItem = new QStandardItem;
			typeEnableItem->setFlags(Qt::ItemIsEnabled);

			FTypeItems.insert(it.key(),typeNameItem);
			FModel.invisibleRootItem()->appendRow(QList<QStandardItem *>() << typeNameItem << typeEnableItem);

			for (int index =0; KindsList[index].kind!=0; index++)
			{
				if ((it->kindMask & KindsList[index].kind)>0)
				{
					QStandardItem *kindNameItem = new QStandardItem(KindsList[index].name);
					kindNameItem->setFlags(Qt::ItemIsEnabled);
					kindNameItem->setData(index,MDR_SORT);

					QStandardItem *kindEnableItem = new QStandardItem();
					kindEnableItem->setFlags(Qt::ItemIsEnabled);
					kindEnableItem->setTextAlignment(Qt::AlignCenter);
					kindEnableItem->setCheckable(true);
					kindEnableItem->setCheckState(Qt::PartiallyChecked);
					kindEnableItem->setData(it.key(),MDR_TYPE);
					kindEnableItem->setData(KindsList[index].kind,MDR_KIND);

					typeNameItem->appendRow(QList<QStandardItem *>() << kindNameItem << kindEnableItem);
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

void NotifyOptionsWidget::setItemItalic(QStandardItem *AItem, bool AItalic) const
{
	QFont font = AItem->font();
	font.setItalic(AItalic);
	AItem->setFont(font);
}

void NotifyOptionsWidget::onRestoreDefaultsClicked()
{
	for(QMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); ++it)
	{
		QStandardItem *typeNameItem = it.value();
		ushort kinds = !it.key().isEmpty() ? FNotifications->notificationType(it.key()).kindDefs : 0xFFFF;
		for (int row=0; row<typeNameItem->rowCount(); row++)
		{
			QStandardItem *kindEnableItem = typeNameItem->child(row, COL_ENABLE);
			kindEnableItem->setCheckState((kinds & (ushort)kindEnableItem->data(MDR_KIND).toInt())>0 ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void NotifyOptionsWidget::onModelItemChanged(QStandardItem *AItem)
{
	if (FBlockChangesCheck<=0 && AItem->column()==COL_ENABLE)
	{
		FBlockChangesCheck++;

		QStandardItem *typeNameItem = AItem->parent();
		QString typeId = AItem->data(MDR_TYPE).toString();
		ushort kindId = (ushort)AItem->data(MDR_KIND).toInt();

		bool enabled = AItem->checkState()==Qt::Checked;
		QStandardItem *kindNameItem = typeNameItem->child(AItem->row(),COL_NAME);
		ushort kinds = !typeId.isEmpty() ? FNotifications->notificationType(typeId).kindDefs : 0xFFFF;
		setItemItalic(kindNameItem,((kinds & kindId)>0)!=enabled);

		if (typeId.isEmpty())
		{
			for(QMap<QString, QStandardItem *>::const_iterator it=FTypeItems.constBegin(); it!=FTypeItems.constEnd(); ++it)
			{
				if (!it.key().isEmpty())
				{
					QStandardItem *typeNameItem = it.value();
					for (int row=0; row<typeNameItem->rowCount(); row++)
					{
						QStandardItem *kindEnableItem = typeNameItem->child(row, COL_ENABLE);
						if (kindId == (ushort)kindEnableItem->data(MDR_KIND).toInt())
						{
							QStandardItem *kindNameItem = typeNameItem->child(row, COL_NAME);
							setItemGray(kindNameItem,!enabled);
						}
					}
				}
			}
		}

		FBlockChangesCheck--;
		emit modified();
	}
}
