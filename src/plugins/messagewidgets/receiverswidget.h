#ifndef RECEIVERSWIDGET_H
#define RECEIVERSWIDGET_H

#include <QHash>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexkindorders.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imessageprocessor.h>
#include <utils/advanceditemdelegate.h>
#include <utils/options.h>
#include "ui_receiverswidget.h"

class ReceiversSortSearchProxyModel : 
	public QSortFilterProxyModel
{
public:
	ReceiversSortSearchProxyModel(QObject *AParent);
	bool isOfflineContactsVisible() const;
	void setOfflineContactsVisible(bool AVisible);
protected:
	bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight)  const;
	bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
	bool FOfflineVisible;
};

class ReceiversWidget :
	public QWidget,
	public IMessageReceiversWidget,
	public AdvancedItemSortHandler
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageReceiversWidget);
public:
	ReceiversWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent);
	~ReceiversWidget();
	// IMessageWidget
	virtual QWidget *instance() { return this; }
	virtual bool isVisibleOnWindow() const;
	virtual IMessageWindow *messageWindow() const;
	// IMessageReceiversWidget
	virtual QList<Jid> availStreams() const;
	virtual QTreeView *receiversView() const;
	virtual AdvancedItemModel *receiversModel() const;
	virtual QModelIndex mapModelToView(QStandardItem *AItem);
	virtual QStandardItem *mapViewToModel(const QModelIndex &AIndex);
	virtual void contextMenuForItems(QList<QStandardItem *> AItems, Menu *AMenu);
	virtual QMultiMap<Jid, Jid> selectedAddresses() const;
	virtual void setGroupSelection(const Jid &AStreamJid, const QString &AGroup, bool ASelected);
	virtual void setAddressSelection(const Jid &AStreamJid, const Jid &AContactJid, bool ASelected);
	virtual void clearSelection();
signals:
	void availStreamsChanged();
	void addressSelectionChanged();
	void contextMenuForItemsRequested(QList<QStandardItem *> AItems, Menu *AMenu);
protected:
	void initialize();
	void createStreamItems(const Jid &AStreamJid);
	void destroyStreamItems(const Jid &AStreamJid);
	QStandardItem *getStreamItem(const Jid &AStreamJid);
	QStandardItem *getGroupItem(const Jid &AStreamJid, const QString &AGroup, int AGroupOrder);
	QList<QStandardItem *> findContactItems(const Jid &AStreamJid, const Jid &AContactJid) const;
	QStandardItem *findContactItem(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const;
	QStandardItem *getContactItem(const Jid &AStreamJid, const Jid &AContactJid, const QString &AName, const QString &AGroup, int AGroupOrder);
protected:
	void deleteItemLater(QStandardItem *AItem);
	void updateCheckState(QStandardItem *AItem);
	void updateContactItemsPresence(const Jid &AStreamJid, const Jid &AContactJid);
protected:
	Jid findAvailStream(const Jid &AStreamJid) const;
	void selectionLoad(const QString &AFileName);
	void selectionSave(const QString &AFileName);
	void selectAllContacts(QList<QStandardItem *> AParents);
	void selectOnlineContacts(QList<QStandardItem *> AParents);
	void selectNotBusyContacts(QList<QStandardItem *> AParents);
	void selectNoneContacts(QList<QStandardItem *> AParents);
	void expandAllChilds(QList<QStandardItem *> AParents);
	void collapseAllChilds(QList<QStandardItem *> AParents);
	void restoreExpandState(QList<QStandardItem *> AParents);
protected slots:
	void onModelItemInserted(QStandardItem *AItem);
	void onModelItemRemoving(QStandardItem *AItem);
	void onModelItemDataChanged(QStandardItem *AItem, int ARole);
protected slots:
	void onViewIndexExpanded(const QModelIndex &AIndex);
	void onViewIndexCollapsed(const QModelIndex &AIndex);
	void onViewContextMenuRequested(const QPoint &APos);
	void onViewModelRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
protected slots:
	void onActiveStreamAppended(const Jid &AStreamJid);
	void onActiveStreamRemoved(const Jid &AStreamJid);
	void onPresenceOpened(IPresence *APresence);
	void onPresenceClosed(IPresence *APresence);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
protected slots:
	void onSelectionLast();
	void onSelectionLoad();
	void onSelectionSave();
	void onSelectAllContacts();
	void onSelectOnlineContacts();
	void onSelectNotBusyContacts();
	void onSelectNoneContacts();
	void onExpandAllChilds();
	void onCollapseAllChilds();
	void onHideOfflineContacts();
protected slots:
	void onDeleteDelayedItems();
	void onStartSearchContacts();
private:
	Ui::ReceiversWidgetClass ui;
private:
	IStatusIcons *FStatusIcons;
	IRostersModel *FRostersModel;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IAccountManager *FAccountManager;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
private:
	QList<Jid> FReceivers;
	IMessageWindow *FWindow;
	AdvancedItemModel *FModel;
	ReceiversSortSearchProxyModel *FProxyModel;
private:
	QTimer FSelectionSignalTimer;
	QList<QStandardItem *> FDeleteDelayed;
	QMap<Jid, QStandardItem *> FStreamItems;
	QMap<Jid, QMap<QString, QStandardItem *> > FGroupItems;
	QMap<Jid, QMultiHash<Jid, QStandardItem *> > FContactItems;
};

Q_DECLARE_METATYPE(QList<QStandardItem *>)

#endif // RECEIVERSWIDGET_H
