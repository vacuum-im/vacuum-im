#include "multiuserview.h"

#include <QToolTip>
#include <QHelpEvent>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <definitions/multiuseritemkinds.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/multiuseritemlabels.h>
#include <definitions/multiuserdataholderorders.h>
#include <definitions/multiusersorthandlerorders.h>
#include <utils/logger.h>

MultiUserView::MultiUserView(IMultiUserChat *AMultiChat, QWidget *AParent) : QTreeView(AParent)
{
	setIndentation(0);
	setRootIsDecorated(false);
	setEditTriggers(NoEditTriggers);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	FViewMode = -1;
	FAvatarSize = 24;

	header()->hide();
	header()->setStretchLastSection(true);

	FDelegate = new AdvancedItemDelegate(this);
	FDelegate->setVertialSpacing(1);
	FDelegate->setHorizontalSpacing(2);
	FDelegate->setItemsRole(MUDR_LABEL_ITEMS);
	FDelegate->setDefaultBranchItemEnabled(true);
	FDelegate->setBlinkMode(AdvancedItemDelegate::BlinkFade);
	setItemDelegate(FDelegate);

	FModel = new AdvancedItemModel(this);
	FModel->setDelayedDataChangedSignals(true);
	FModel->setRecursiveParentDataChangedSignals(true);
	FModel->insertItemDataHolder(MUDHO_MULTIUSERCHAT, this);
	FModel->insertItemSortHandler(MUSHO_MULTIUSERVIEW, this);
	setModel(FModel);

	FBlinkTimer.setSingleShot(false);
	FBlinkTimer.setInterval(FDelegate->blinkInterval());
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimerTimeout()));

	FMultiChat = AMultiChat;
	connect(FMultiChat->instance(),SIGNAL(userChanged(IMultiUser *, int, const QVariant &)),SLOT(onMultiUserChanged(IMultiUser *, int, const QVariant &)));

	if (FStatusIcons)
		connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));

	if (FAvatars)
		connect(FAvatars->instance(),SIGNAL(avatarChanged(const Jid &)),SLOT(onAvatarChanged(const Jid &)));
}

MultiUserView::~MultiUserView()
{

}

QList<int> MultiUserView::advancedItemDataRoles(int AOrder) const
{
	if (AOrder == MUDHO_MULTIUSERCHAT)
	{
		static const QList<int> roles = QList<int>() 
			<< MUDR_STREAM_JID << MUDR_USER_JID << MUDR_REAL_JID
			<< MUDR_NICK << MUDR_ROLE << MUDR_AFFILIATION
			<< MUDR_AVATAR_IMAGE;
		return roles;
	}
	return QList<int>();
}

QVariant MultiUserView::advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const
{
	if (AOrder == MUDHO_MULTIUSERCHAT)
	{
		IMultiUser *user = FItemUser.value(AItem);
		if (user != NULL)
		{
			switch (ARole)
			{
			case MUDR_STREAM_JID:
				return user->streamJid().full();
			case MUDR_USER_JID:
				return user->userJid().full();
			case MUDR_REAL_JID:
				return user->realJid().full();
			case MUDR_NICK:
				return user->nick();
			case MUDR_ROLE:
				return user->role();
			case MUDR_AFFILIATION:
				return user->affiliation();
			case MUDR_AVATAR_IMAGE:
				return FAvatars!=NULL ? FAvatars->visibleAvatarImage(FAvatars->avatarHash(user->userJid()),FAvatarSize) : QVariant();
			}
		}
	}
	return QVariant();
}

#define SORT_RESULT(less) ((less) ? AdvancedItemSortHandler::LessThen : AdvancedItemSortHandler::NotLessThen)
AdvancedItemSortHandler::SortResult MultiUserView::advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const
{
	if (AOrder==MUSHO_MULTIUSERVIEW && ALeft->data(MUDR_KIND).toInt()==MUIK_USER && ARight->data(MUDR_KIND).toInt()==MUIK_USER)
	{
		static const QList<QString> roles = QList<QString>() << MUC_ROLE_MODERATOR << MUC_ROLE_PARTICIPANT << MUC_ROLE_VISITOR << MUC_ROLE_NONE;
		static const QList<QString> affiliations = QList<QString>() << MUC_AFFIL_OWNER << MUC_AFFIL_ADMIN << MUC_AFFIL_MEMBER << MUC_AFFIL_OUTCAST << MUC_AFFIL_NONE;

		IMultiUser *leftUser = FItemUser.value(ALeft);
		IMultiUser *rightUser = FItemUser.value(ARight);
		if (leftUser!=NULL && rightUser!=NULL)
		{
			int leftAffilIndex =  affiliations.indexOf(leftUser->affiliation());
			int rightAffilIndex = affiliations.indexOf(rightUser->affiliation());
			if (leftAffilIndex != rightAffilIndex)
				return SORT_RESULT(leftAffilIndex < rightAffilIndex);

			int leftRoleIndex =  roles.indexOf(leftUser->role());
			int rightRoleIndex = roles.indexOf(rightUser->role());
			if (leftRoleIndex != rightRoleIndex)
				return SORT_RESULT(leftRoleIndex < rightRoleIndex);
		}

		return SORT_RESULT(QString::localeAwareCompare(ALeft->data(Qt::DisplayRole).toString(), ARight->data(Qt::DisplayRole).toString()) < 0);
	}
	return AdvancedItemSortHandler::Undefined;
}

int MultiUserView::viewMode() const
{
	return FViewMode;
}

void MultiUserView::setViewMode(int AMode)
{
	if (FViewMode != AMode)
	{
		LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Changing view mode from %1 to %2, room=%3").arg(FViewMode).arg(AMode).arg(FMultiChat->roomJid().full()));

		FViewMode = AMode;

		foreach(QStandardItem *item, FUserItem)
			updateItemNotify(item);

		if (FViewMode != IMultiUserView::ViewCompact)
		{
			AdvancedDelegateItem avatarLabel;
			avatarLabel.d->id = MUIL_MULTIUSERCHAT_AVATAR;
			avatarLabel.d->kind = AdvancedDelegateItem::CustomData;
			avatarLabel.d->data = MUDR_AVATAR_IMAGE;
			insertGeneralLabel(avatarLabel);
		}
		else
		{
			removeGeneralLabel(MUIL_MULTIUSERCHAT_AVATAR);
		}

		if (FAvatars)
		{
			if (FViewMode == IMultiUserView::ViewFull)
				FAvatarSize = FAvatars->avatarSize(IAvatars::AvatarNormal);
			else
				FAvatarSize = FAvatars->avatarSize(IAvatars::AvatarSmall);
		}

		emit viewModeChanged(FViewMode);
	}
}

AdvancedItemModel *MultiUserView::model() const
{
	return FModel;
}

AdvancedItemDelegate *MultiUserView::itemDelegate() const
{
	return FDelegate;
}

IMultiUser *MultiUserView::findItemUser(const QStandardItem *AItem) const
{
	return FItemUser.value(AItem);
}

QStandardItem *MultiUserView::findUserItem(const IMultiUser *AUser) const
{
	return FUserItem.value(AUser);
}

QModelIndex MultiUserView::indexFromItem(const QStandardItem *AItem) const
{
	return FModel->indexFromItem(AItem);
}

QStandardItem *MultiUserView::itemFromIndex(const QModelIndex &AIndex) const
{
	return FModel->itemFromIndex(AIndex);
}

AdvancedDelegateItem MultiUserView::generalLabel(quint32 ALabelId) const
{
	return FGeneralLabels.value(ALabelId);
}

void MultiUserView::insertGeneralLabel(const AdvancedDelegateItem &ALabel)
{
	if (ALabel.d->id != AdvancedDelegateItem::NullId)
	{
		LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Inserting general label, label=%1, room=%2").arg(ALabel.d->id).arg(FMultiChat->roomJid().bare()));
		
		FGeneralLabels.insert(ALabel.d->id,ALabel);
		foreach(QStandardItem *item, FUserItem)
			insertItemLabel(ALabel,item);
	}
	else
	{
		REPORT_ERROR("Failed to insert general label: Invalid label");
	}
}

void MultiUserView::removeGeneralLabel(quint32 ALabelId)
{
	if (ALabelId != AdvancedDelegateItem::NullId)
	{
		LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Removing general label, label=%1, room=%2").arg(ALabelId).arg(FMultiChat->roomJid().bare()));
		FGeneralLabels.remove(ALabelId);
		removeItemLabel(ALabelId);
	}
	else
	{
		REPORT_ERROR("Failed to remove general label: Invalid label");
	}
}

void MultiUserView::insertItemLabel(const AdvancedDelegateItem &ALabel, QStandardItem *AItem)
{
	if (ALabel.d->id != AdvancedDelegateItem::NullId)
	{
		if (!FLabelItems.contains(ALabel.d->id,AItem))
			FLabelItems.insertMulti(ALabel.d->id,AItem);

		if ((ALabel.d->flags & AdvancedDelegateItem::Blink) == 0)
			FBlinkItems.remove(ALabel.d->id,AItem);
		else if (!FBlinkItems.contains(ALabel.d->id, AItem))
			FBlinkItems.insertMulti(ALabel.d->id,AItem);
		updateBlinkTimer();

		AdvancedDelegateItems labelItems = AItem->data(MUDR_LABEL_ITEMS).value<AdvancedDelegateItems>();
		labelItems.insert(ALabel.d->id, ALabel);
		AItem->setData(QVariant::fromValue<AdvancedDelegateItems>(labelItems),MUDR_LABEL_ITEMS);
	}
	else
	{
		REPORT_ERROR("Failed to insert item label: Invalid label");
	}
}

void MultiUserView::removeItemLabel(quint32 ALabelId, QStandardItem *AItem)
{
	if (ALabelId != AdvancedDelegateItem::NullId)
	{
		if (AItem == NULL)
		{
			foreach(QStandardItem *item, FLabelItems.values(ALabelId))
				removeItemLabel(ALabelId,item);
		}
		else if (FLabelItems.contains(ALabelId,AItem))
		{
			FLabelItems.remove(ALabelId,AItem);
			FBlinkItems.remove(ALabelId,AItem);
			updateBlinkTimer();

			AdvancedDelegateItems labelItems = AItem->data(MUDR_LABEL_ITEMS).value<AdvancedDelegateItems>();
			labelItems.remove(ALabelId);
			AItem->setData(QVariant::fromValue<AdvancedDelegateItems>(labelItems),MUDR_LABEL_ITEMS);
		}
	}
	else
	{
		REPORT_ERROR("Failed to remove item label: Invalid label");
	}
}

QRect MultiUserView::labelRect(quint32 ALabeld, const QModelIndex &AIndex) const
{
	return FDelegate->itemRect(ALabeld,indexOption(AIndex),AIndex);
}

quint32 MultiUserView::labelAt(const QPoint &APoint, const QModelIndex &AIndex) const
{
	return FDelegate->itemAt(APoint,indexOption(AIndex),AIndex);
}

QStandardItem *MultiUserView::notifyItem(int ANotifyId) const
{
	return FItemNotifies.key(ANotifyId);
}

QList<int> MultiUserView::itemNotifies(QStandardItem *AItem) const
{
	QMultiMap<int, int> orderMap;
	foreach(int notifyId, FItemNotifies.values(AItem))
		orderMap.insertMulti(FNotifies.value(notifyId).order, notifyId);
	return orderMap.values();
}

IMultiUserViewNotify MultiUserView::itemNotify(int ANotifyId) const
{
	return FNotifies.value(ANotifyId);
}

int MultiUserView::insertItemNotify(const IMultiUserViewNotify &ANotify, QStandardItem *AItem)
{
	static int NotifyId = 0;
	do NotifyId = qMax(++NotifyId, 1); while (FNotifies.contains(NotifyId));

	LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Inserting item notify, notify=%1, order=%2, flags=%3, room=%4").arg(NotifyId).arg(ANotify.order).arg(ANotify.flags).arg(FMultiChat->roomJid().bare()));

	FNotifies.insert(NotifyId,ANotify);
	FItemNotifies.insertMulti(AItem,NotifyId);
	updateItemNotify(AItem);

	emit itemNotifyInserted(NotifyId);

	return NotifyId;
}

void MultiUserView::activateItemNotify(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Activating item notify, notify=%1, room=%2").arg(ANotifyId).arg(FMultiChat->roomJid().bare()));
		emit itemNotifyActivated(ANotifyId);
	}
}

void MultiUserView::removeItemNotify(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Removing item notify, notify=%1, room=%2").arg(ANotifyId).arg(FMultiChat->roomJid().bare()));

		QStandardItem *item = FItemNotifies.key(ANotifyId);
		FNotifies.remove(ANotifyId);
		FItemNotifies.remove(item);
		updateItemNotify(item);

		emit itemNotifyRemoved(ANotifyId);
	}
}

void MultiUserView::contextMenuForItem(QStandardItem *AItem, Menu *AMenu)
{
	LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Requesting context menu for item, user=%1").arg(AItem->data(MUDR_USER_JID).toString()));
	emit itemContextMenu(AItem,AMenu);
}

void MultiUserView::toolTipsForItem(QStandardItem *AItem, QMap<int,QString> &AToolTips)
{
	LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Requesting tool tips for item, user=%1").arg(AItem->data(MUDR_USER_JID).toString()));
	emit itemToolTips(AItem,AToolTips);
}

void MultiUserView::updateBlinkTimer()
{
	if (!FBlinkTimer.isActive() && !FBlinkItems.isEmpty())
		FBlinkTimer.start();
	else if (FBlinkTimer.isActive() && FBlinkItems.isEmpty())
		FBlinkTimer.stop();
}

void MultiUserView::updateUserItem(IMultiUser *AUser)
{
	QStandardItem *userItem = FUserItem.value(AUser);
	if (userItem)
	{
		QIcon userIcon;
		QColor userColor;
		QFont userFont = userItem->font();
		IPresenceItem userPresence = AUser->presence();

		// User Role
		if (AUser->role() == MUC_ROLE_MODERATOR)
		{
			userFont.setBold(true);
			userColor = palette().color(QPalette::Active, QPalette::Text);
		}
		else if (AUser->role() == MUC_ROLE_PARTICIPANT)
		{
			userFont.setBold(false);
			userColor = palette().color(QPalette::Active, QPalette::Text);
		}
		else
		{
			userFont.setBold(false);
			userColor = palette().color(QPalette::Disabled, QPalette::Text);
		}

		// User affiliation
		QString affilation = AUser->affiliation();
		if (AUser->affiliation() == MUC_AFFIL_OWNER)
		{
			userFont.setStrikeOut(false);
			userFont.setUnderline(true);
			userFont.setItalic(false);
		}
		else if (AUser->affiliation() == MUC_AFFIL_ADMIN)
		{
			userFont.setStrikeOut(false);
			userFont.setUnderline(false);
			userFont.setItalic(false);
		}
		else if (AUser->affiliation() == MUC_AFFIL_MEMBER)
		{
			userFont.setStrikeOut(false);
			userFont.setUnderline(false);
			userFont.setItalic(false);
		}
		else if (AUser->affiliation() == MUC_AFFIL_OUTCAST)
		{
			userFont.setStrikeOut(true);
			userFont.setUnderline(false);
			userFont.setItalic(false);
		}
		else
		{
			userFont.setStrikeOut(false);
			userFont.setUnderline(false);
			userFont.setItalic(true);
		}

		// User status
		userIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AUser->userJid(),userPresence.show,QString::null,false) : QIcon();

		userItem->setIcon(userIcon);
		userItem->setText(AUser->nick());

		userItem->setData(userPresence.show, MUDR_PRESENCE_SHOW);
		userItem->setData(userPresence.status, MUDR_PRESENCE_STATUS);

		AdvancedDelegateItem nickLabel = userItem->data(MUDR_LABEL_ITEMS).value<AdvancedDelegateItems>().value(MUIL_MULTIUSERCHAT_NICK);
		if (nickLabel.d->hints.value(AdvancedDelegateItem::FontHint)!=userFont || nickLabel.d->hints.value(AdvancedDelegateItem::Foreground)!=userColor)
		{
			nickLabel.d->hints.insert(AdvancedDelegateItem::FontHint,userFont);
			nickLabel.d->hints.insert(AdvancedDelegateItem::Foreground,userColor);
			insertItemLabel(nickLabel,userItem);
		}
	}
}

void MultiUserView::updateItemNotify(QStandardItem *AItem)
{
	int newNotifyId = itemNotifies(AItem).value(0);
	IMultiUserViewNotify newNotify = FNotifies.value(newNotifyId);
	AdvancedDelegateItems labels = AItem->data(MUDR_LABEL_ITEMS).value<AdvancedDelegateItems>();

	AdvancedDelegateItem iconLabel = labels.value(MUIL_MULTIUSERCHAT_ICON);
	iconLabel.d->data = !newNotify.icon.isNull() ? QVariant(newNotify.icon) : QVariant(Qt::DecorationRole);
	iconLabel.d->flags = (newNotify.flags & IMultiUserViewNotify::Blink)>0 ? AdvancedDelegateItem::Blink : 0;
	insertItemLabel(iconLabel,AItem);

	AdvancedDelegateItem statusLabel = labels.value(MUIL_MULTIUSERCHAT_STATUS);
	if (!newNotify.footer.isNull())
		statusLabel.d->data = newNotify.footer;
	else if (FViewMode == IMultiUserView::ViewFull)
		statusLabel.d->data = MUDR_PRESENCE_STATUS;
	else
		statusLabel.d->data = QVariant();
	insertItemLabel(statusLabel,AItem);
}

void MultiUserView::repaintUserItem(const QStandardItem *AItem)
{
	QRect rect = visualRect(indexFromItem(AItem)).adjusted(1,1,-1,-1);
	if (!rect.isEmpty())
		viewport()->repaint(rect);
}

QStyleOptionViewItemV4 MultiUserView::indexOption(const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 option = viewOptions();

	option.index = AIndex;
	option.rect = visualRect(AIndex);
	option.showDecorationSelected = false;

	option.widget = this;
	option.locale = locale();
	option.locale.setNumberOptions(QLocale::OmitGroupSeparator);

	if (isExpanded(AIndex))
		option.state |= QStyle::State_Open;
	if (hasFocus() && currentIndex()==AIndex)
		option.state |= QStyle::State_HasFocus;
	if (selectedIndexes().contains(AIndex))
		option.state |= QStyle::State_Selected;
	if ((AIndex.flags() & Qt::ItemIsEnabled) == 0)
		option.state &= ~QStyle::State_Enabled;
	if (indexAt(viewport()->mapFromGlobal(QCursor::pos())) == AIndex)
		option.state |= QStyle::State_MouseOver;
	if (model() && model()->hasChildren(AIndex))
		option.state |= QStyle::State_Children;
	option.state &= ~(QStyle::State_Sibling|QStyle::State_Item);

	if (wordWrap())
		option.features |= QStyleOptionViewItemV2::WrapText;

	return option;
}

bool MultiUserView::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(AEvent);
		QStandardItem *userItem = itemFromIndex(indexAt(menuEvent->pos()));
		if (userItem)
		{
			Menu *menu = new Menu(this);
			menu->setAttribute(Qt::WA_DeleteOnClose,true);
			contextMenuForItem(userItem,menu);

			if (!menu->isEmpty())
				menu->popup(menuEvent->globalPos());
			else
				delete menu;
		}
		AEvent->accept();
		return true;
	}
	else if (AEvent->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent);
		QStandardItem *userItem = itemFromIndex(indexAt(helpEvent->pos()));
		if (userItem)
		{
			QMap<int,QString> toolTips;
			toolTipsForItem(userItem,toolTips);
			
			if (!toolTips.isEmpty())
			{
				QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTips.values()).join("<p/><nbsp>"));
				QToolTip::showText(helpEvent->globalPos(),tooltip,this);
			}
		}
		AEvent->accept();
		return true;
	}
	return QTreeView::event(AEvent);
}

void MultiUserView::onMultiUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore)
{
	Q_UNUSED(ABefore);
	QStandardItem *userItem = FUserItem.value(AUser);
	if (AData == MUDR_PRESENCE)
	{
		IPresenceItem presence = AUser->presence();
		
		if (presence.show!=IPresence::Offline && presence.show!=IPresence::Error)
		{
			if (userItem == NULL)
			{
				LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Creating user item, user=%1").arg(AUser->userJid().full()));

				userItem = new AdvancedItem(AUser->nick());
				userItem->setData(MUIK_USER,MUDR_KIND);
				FUserItem.insert(AUser,userItem);
				FItemUser.insert(userItem,AUser);

				AdvancedDelegateItem iconLabel;
				iconLabel.d->id = MUIL_MULTIUSERCHAT_ICON;
				iconLabel.d->kind = AdvancedDelegateItem::Decoration;
				iconLabel.d->data = Qt::DecorationRole;
				insertItemLabel(iconLabel,userItem);

				AdvancedDelegateItem nickLabel;
				nickLabel.d->id = MUIL_MULTIUSERCHAT_NICK;
				nickLabel.d->kind = AdvancedDelegateItem::Display;
				nickLabel.d->data = Qt::DisplayRole;
				insertItemLabel(nickLabel,userItem);

				AdvancedDelegateItem statusLabel;
				statusLabel.d->id = MUIL_MULTIUSERCHAT_STATUS;
				statusLabel.d->kind = AdvancedDelegateItem::CustomData;
				statusLabel.d->data = FViewMode==IMultiUserView::ViewFull ? QVariant(MUDR_PRESENCE_STATUS) : QVariant();
				statusLabel.d->hints.insert(AdvancedDelegateItem::FontSizeDelta,-1);
				statusLabel.d->hints.insert(AdvancedDelegateItem::FontItalic,true);
				insertItemLabel(statusLabel,userItem);

				foreach(const AdvancedDelegateItem &label, FGeneralLabels)
					insertItemLabel(label,userItem);
				updateUserItem(AUser);
				
				FModel->appendRow(userItem);
				FModel->sort(0);
			}
			else
			{
				updateUserItem(AUser);
			}
		}
		else if (userItem != NULL)
		{
			LOG_STRM_DEBUG(FMultiChat->streamJid(),QString("Destroying user item, user=%1").arg(AUser->userJid().full()));

			foreach(int notifyId, FItemNotifies.values(userItem))
				removeItemNotify(notifyId);

			foreach(int labelId, FLabelItems.keys(userItem))
				removeItemLabel(labelId,userItem);

			qDeleteAll(FModel->takeRow(userItem->row()));
			FUserItem.remove(AUser);
			FItemUser.remove(userItem);

			userItem = NULL;
		}
	}
	else if (AData == MUDR_NICK)
	{
		updateUserItem(AUser);
		FModel->sort(0);
	}
	else if (AData == MUDR_ROLE)
	{
		updateUserItem(AUser);
		FModel->sort(0);
	}
	else if (AData == MUDR_AFFILIATION)
	{
		updateUserItem(AUser);
		FModel->sort(0);
	}

	if (userItem != NULL)
		emitItemDataChanged(userItem, AData);
}

void MultiUserView::onBlinkTimerTimeout()
{
	if (FDelegate->blinkNeedUpdate())
	{
		for (QMultiMap<quint32, QStandardItem *>::const_iterator it=FBlinkItems.constBegin(); it!=FBlinkItems.constEnd(); ++it)
			repaintUserItem(it.value());
	}
}

void MultiUserView::onStatusIconsChanged()
{
	foreach(IMultiUser *user, FItemUser)
		updateUserItem(user);
}

void MultiUserView::onAvatarChanged(const Jid &AContactJid)
{
	if (FMultiChat->roomJid() == AContactJid.bare())
	{
		QStandardItem *userItem = FUserItem.value(FMultiChat->findUser(AContactJid.resource()));
		if (userItem != NULL)
			emitItemDataChanged(userItem, MUDR_AVATAR_IMAGE);
	}
}
