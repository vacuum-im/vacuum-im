#ifndef MULTIUSERVIEW_H
#define MULTIUSERVIEW_H

#include <interfaces/imultiuserchat.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iavatars.h>
#include <utils/pluginhelper.h>

class MultiUserView :
	public QTreeView,
	public IMultiUserView,
	public AdvancedItemDataHolder,
	public AdvancedItemSortHandler
{
	Q_OBJECT;
	Q_INTERFACES(IMultiUserView);
public:
	MultiUserView(IMultiUserChat *AMultiChat, QWidget *AParent = NULL);
	~MultiUserView();
	virtual QTreeView *instance() { return this; }
	// AdvancedItemDataHolder
	virtual QList<int> advancedItemDataRoles(int AOrder) const;
	virtual QVariant advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const;
	// AdvancedItemSortHandler
	virtual SortResult advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const;
	// IMultiUserView
	virtual int viewMode() const;
	virtual void setViewMode(int AMode);
	// Items
	virtual AdvancedItemModel *model() const;
	virtual AdvancedItemDelegate *itemDelegate() const;
	virtual IMultiUser *findItemUser(const QStandardItem *AItem) const;
	virtual QStandardItem *findUserItem(const IMultiUser *AUser) const;
	virtual QModelIndex indexFromItem(const QStandardItem *AItem) const;
	virtual QStandardItem *itemFromIndex(const QModelIndex &AIndex) const;
	// Labels
	virtual AdvancedDelegateItem generalLabel(quint32 ALabelId) const;
	virtual void insertGeneralLabel(const AdvancedDelegateItem &ALabel);
	virtual void removeGeneralLabel(quint32 ALabelId);
	virtual void insertItemLabel(const AdvancedDelegateItem &ALabel, QStandardItem *AItem);
	virtual void removeItemLabel(quint32 ALabelId, QStandardItem *AItem = NULL);
	virtual QRect labelRect(quint32 ALabeld, const QModelIndex &AIndex) const;
	virtual quint32 labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
	// Notify
	virtual QStandardItem *notifyItem(int ANotifyId) const;
	virtual QList<int> itemNotifies(QStandardItem *AItem) const;
	virtual IMultiUserViewNotify itemNotify(int ANotifyId) const;
	virtual int insertItemNotify(const IMultiUserViewNotify &ANotify, QStandardItem *AItem);
	virtual void activateItemNotify(int ANotifyId);
	virtual void removeItemNotify(int ANotifyId);
	// Context
	virtual void contextMenuForItem(QStandardItem *AItem, Menu *AMenu);
	virtual void toolTipsForItem(QStandardItem *AItem, QMap<int,QString> &AToolTips);
signals:
	void viewModeChanged(int AMode);
	// Notify
	void itemNotifyInserted(int ANotifyId);
	void itemNotifyActivated(int ANotifyId);
	void itemNotifyRemoved(int ANotfyId);
	// Context
	void itemContextMenu(QStandardItem *AItem, Menu *AMenu);
	void itemToolTips(QStandardItem *AItem, QMap<int,QString> &AToolTips);
protected:
	void updateBlinkTimer();
	void updateUserItem(IMultiUser *AUser);
	void updateItemNotify(QStandardItem *AItem);
	void repaintUserItem(const QStandardItem *AItem);
	QStyleOptionViewItemV4 indexOption(const QModelIndex &AIndex) const;
protected:
	bool event(QEvent *AEvent);
protected slots:
	void onMultiUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore);
protected slots:
	void onBlinkTimerTimeout();
	void onStatusIconsChanged();
	void onAvatarChanged(const Jid &AContactJid);
private:
	PluginPointer<IAvatars> FAvatars;
	PluginPointer<IStatusIcons> FStatusIcons;
	PluginPointer<IMultiUserChat> FMultiChat;
private:
	QTimer FBlinkTimer;
	QMultiMap<quint32, QStandardItem *> FBlinkItems;
	QMultiMap<quint32, QStandardItem *> FLabelItems;
	QMap<quint32, AdvancedDelegateItem> FGeneralLabels;
private:
	QMap<int, IMultiUserViewNotify> FNotifies;
	QMultiMap<QStandardItem *, int> FItemNotifies;
private:
	int FViewMode;
	int FAvatarSize;
	AdvancedItemModel *FModel;
	AdvancedItemDelegate *FDelegate;
	QHash<const IMultiUser *, QStandardItem *> FUserItem;
	QHash<const QStandardItem *, IMultiUser *> FItemUser;
};

#endif // MULTIUSERVIEW_H
