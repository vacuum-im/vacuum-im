#ifndef EDITUSERSLISTDIALOG_H
#define EDITUSERSLISTDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/imultiuserchat.h>
#include <utils/advanceditemdelegate.h>
#include "ui_edituserslistdialog.h"

class UsersListProxyModel :
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	UsersListProxyModel(QObject *AParent = NULL);
protected:
	bool filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const;
};

class EditUsersListDialog :
	public QDialog
{
	Q_OBJECT;
public:
	EditUsersListDialog(IMultiUserChat *AMultiChat, const QString &AAffiliation=MUC_AFFIL_NONE, QWidget *AParent = NULL);
	~EditUsersListDialog();
protected:
	QString currentAffiliation() const;
	QList<IMultiUserListItem> deltaList() const;
	QString affiliatioName(const QString &AAffiliation) const;
	void updateAffiliationTabNames() const;
protected:
	QList<QStandardItem *> selectedModelItems() const;
	QStandardItem *createModelItem(const Jid &ARealJid) const;
	void updateModelItem(QStandardItem *AModelItem, const IMultiUserListItem &AListItem) const;
	void applyListItems(const QList<IMultiUserListItem> &AListItems);
protected slots:
	void onAddClicked();
	void onDeleteClicked();
	void onMoveUserActionTriggered();
	void onSearchLineEditSearchStart();
	void onCurrentAffiliationChanged(int ATabIndex);
	void onItemsTableContextMenuRequested(const QPoint &APos);
	void onDialogButtonBoxButtonClicked(QAbstractButton *AButton);
protected slots:
	void onMultiChatRequestFailed(const QString &AId, const XmppError &AError);
	void onMultiChatListLoaded(const QString &AId, const QList<IMultiUserListItem> &AItems);
	void onMultiChatListUpdated(const QString &AId, const QList<IMultiUserListItem> &AItems);
private:
	Ui::EditUsersListDialogClass ui;
private:
	QStandardItemModel *FModel;
	UsersListProxyModel *FProxy;
	AdvancedItemDelegate *FDelegate;
private:
	IMultiUserChat *FMultiChat;
	QMap<QString, int> FAffilTab;
	QMap<QString, QStandardItem *> FAffilRoot;
private:
	QString FAffilUpdateId;
	QMap<QString, QString> FAffilLoadId;
	QHash<Jid, QStandardItem *> FUserModelItem;
	QHash<Jid, IMultiUserListItem> FUserListItem;
};

#endif // EDITUSERSLISTDIALOG_H
