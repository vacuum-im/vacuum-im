#include "notifykindoptionswidget.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>

enum NotifiesTableColumn {
	NTC_TYPE,
	NTC_SOUND,
	NTC_POPUP,
	NTC_MINIMIZED,
	NTC__COUNT
};

enum NotifiesTableRoles {
	NTR_TYPE = Qt::UserRole,
	NTR_KIND
};

struct NotificationType :	public INotificationType
{
	NotificationType(const INotificationType &AType) : INotificationType(AType) {}
	QString typeId;
};

NotifyKindOptionsWidget::NotifyKindOptionsWidget(INotifications *ANotifications, QWidget *AParent) : QWidget(AParent)
{
	FNotifications = ANotifications;

	tbwNotifies = new QTableWidget(this);
	tbwNotifies->setWordWrap(true);
	tbwNotifies->verticalHeader()->setVisible(false);
	tbwNotifies->horizontalHeader()->setHighlightSections(false);
	tbwNotifies->setSelectionMode(QTableWidget::NoSelection);
	connect(tbwNotifies,SIGNAL(itemChanged(QTableWidgetItem *)),SIGNAL(modified()));

	tbwNotifies->setColumnCount(NTC__COUNT);
	tbwNotifies->setHorizontalHeaderLabels(QStringList() << tr("Event") << "" << "" << "");
	tbwNotifies->horizontalHeader()->setSectionResizeMode(NTC_TYPE,QHeaderView::Stretch);

	tbwNotifies->horizontalHeader()->setSectionResizeMode(NTC_SOUND,QHeaderView::ResizeToContents);
	tbwNotifies->horizontalHeaderItem(NTC_SOUND)->setToolTip(tr("Play sound at the notification"));
	tbwNotifies->horizontalHeaderItem(NTC_SOUND)->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NOTIFICATIONS_SOUNDPLAY));

	tbwNotifies->horizontalHeader()->setSectionResizeMode(NTC_POPUP,QHeaderView::ResizeToContents);
	tbwNotifies->horizontalHeaderItem(NTC_POPUP)->setToolTip(tr("Display a notification in popup window"));
	tbwNotifies->horizontalHeaderItem(NTC_POPUP)->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NOTIFICATIONS_PUPUPWINDOW));

	tbwNotifies->horizontalHeader()->setSectionResizeMode(NTC_MINIMIZED,QHeaderView::ResizeToContents);
	tbwNotifies->horizontalHeaderItem(NTC_MINIMIZED)->setToolTip(tr("Show the corresponding window minimized in the taskbar"));
	tbwNotifies->horizontalHeaderItem(NTC_MINIMIZED)->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NOTIFICATIONS_SHOWMINIMIZED));

	QVBoxLayout *vblLayout = new QVBoxLayout(this);
	vblLayout->addWidget(tbwNotifies);
	vblLayout->setMargin(0);

	QMultiMap<int, NotificationType> orderedTypes;
	ushort visibleKinds = INotification::PopupWindow|INotification::SoundPlay|INotification::ShowMinimized;
	foreach(const QString &typeId, FNotifications->notificationTypes())
	{
		NotificationType notifyType = FNotifications->notificationType(typeId);
		if (!notifyType.title.isEmpty() && (notifyType.kindMask & visibleKinds)>0)
		{
			notifyType.typeId = typeId;
			orderedTypes.insertMulti(notifyType.order,notifyType);
		}
	}

	for (QMultiMap<int, NotificationType>::const_iterator it=orderedTypes.constBegin(); it!=orderedTypes.constEnd(); ++it)
	{
		int row = tbwNotifies->rowCount();
		tbwNotifies->setRowCount(row+1);

		QTableWidgetItem *type = new QTableWidgetItem(it->icon,it->title);
		type->setData(NTR_TYPE, it->typeId);
		type->setFlags(Qt::ItemIsEnabled);
		tbwNotifies->setItem(row,NTC_TYPE,type);

		QTableWidgetItem *sound = new QTableWidgetItem();
		sound->setData(NTR_KIND, INotification::SoundPlay);
		if (it->kindDefs & INotification::SoundPlay)
			sound->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		else
			sound->setFlags(Qt::ItemIsUserCheckable);
		sound->setCheckState(Qt::Unchecked);
		tbwNotifies->setItem(row,NTC_SOUND,sound);

		QTableWidgetItem *popup = new QTableWidgetItem();
		popup->setData(NTR_KIND, INotification::PopupWindow);
		if (it->kindDefs & INotification::PopupWindow)
			popup->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		else
			popup->setFlags(Qt::ItemIsUserCheckable);
		popup->setCheckState(Qt::Unchecked);
		tbwNotifies->setItem(row,NTC_POPUP,popup);

		QTableWidgetItem *minimized = new QTableWidgetItem();
		minimized->setData(NTR_KIND, INotification::ShowMinimized);
		if (it->kindDefs & INotification::ShowMinimized)
			minimized->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		else
			minimized->setFlags(Qt::ItemIsUserCheckable);
		minimized->setCheckState(Qt::Unchecked);
		tbwNotifies->setItem(row,NTC_MINIMIZED,minimized);

		tbwNotifies->verticalHeader()->setSectionResizeMode(row,QHeaderView::ResizeToContents);
	}

	reset();
}

void NotifyKindOptionsWidget::apply()
{
	for (int row=0; row<tbwNotifies->rowCount(); row++)
	{
		QString typeId = tbwNotifies->item(row,NTC_TYPE)->data(NTR_TYPE).toString();
		NotificationType notifyType = FNotifications->notificationType(typeId);

		ushort typeKinds = notifyType.kindDefs & notifyType.kindMask;
		for (int col=1; col<tbwNotifies->columnCount(); col++)
		{
			ushort kind = tbwNotifies->item(row,col)->data(NTR_KIND).toInt();
			if (tbwNotifies->item(row,col)->checkState() == Qt::Checked)
				typeKinds |= kind;
			else
				typeKinds &= ~kind;
		}

		FNotifications->setTypeNotificationKinds(typeId,typeKinds);
	}
}

void NotifyKindOptionsWidget::reset()
{
	for (int row=0; row<tbwNotifies->rowCount(); row++)
	{
		QString typeId = tbwNotifies->item(row,NTC_TYPE)->data(NTR_TYPE).toString();
		ushort typeKinds = FNotifications->typeNotificationKinds(typeId);
		for (int col=1; col<tbwNotifies->columnCount(); col++)
		{
			ushort kind = tbwNotifies->item(row,col)->data(NTR_KIND).toInt();
			tbwNotifies->item(row,col)->setCheckState(typeKinds & kind ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void NotifyKindOptionsWidget::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);

	int minHeight = tbwNotifies->horizontalHeader()->height() + tbwNotifies->frameWidth()*2;
	for (int row=0; row<tbwNotifies->rowCount(); row++)
		minHeight += tbwNotifies->verticalHeader()->sectionSize(row);
	tbwNotifies->setMinimumHeight(minHeight);
}
