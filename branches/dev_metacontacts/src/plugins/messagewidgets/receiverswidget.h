#ifndef RECEIVERSWIDGET_H
#define RECEIVERSWIDGET_H

#include <QHash>
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
#include "ui_receiverswidget.h"

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
	// AdvancedItemSortHandler
	virtual SortResult advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const;
	// IMessageWidget
	virtual QWidget *instance() { return this; }
	virtual bool isVisibleOnWindow() const;
	virtual IMessageWindow *messageWindow() const;
	// IMessageReceiversWidget
	virtual QList<Jid> availStreams() const;
	virtual QTreeView *receiversView() const;
	virtual AdvancedItemModel *receiversModel() const;
	virtual void contextMenuForItem(QStandardItem *AItem, Menu *AMenu);
	virtual QMultiMap<Jid, Jid> selectedAddresses() const;
	virtual void setGroupSelection(const Jid &AStreamJid, const QString &AGroup, bool ASelected);
	virtual void setAddressSelection(const Jid &AStreamJid, const Jid &AContactJid, bool ASelected);
	virtual void clearSelection();
signals:
	void availStreamsChanged();
	void contextMenuForItemRequested(QStandardItem *AItem, Menu *AMenu);
	void addressSelectionChanged(const Jid &AStreamJid, const Jid &AContactJid, bool ASelected);
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
	void selectAllContacts(QStandardItem *AParent);
	void selectOnlineContacts(QStandardItem *AParent);
	void selectNotBusyContacts(QStandardItem *AParent);
	void selectNoneContacts(QStandardItem *AParent);
protected slots:
	void onModelItemInserted(QStandardItem *AItem);
	void onModelItemRemoving(QStandardItem *AItem);
	void onModelItemDataChanged(QStandardItem *AItem, int ARole);
protected slots:
	void onActiveStreamAppended(const Jid &AStreamJid);
	void onActiveStreamRemoved(const Jid &AStreamJid);
	void onPresenceOpened(IPresence *APresence);
	void onPresenceClosed(IPresence *APresence);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
protected slots:
	void onSelectionLoad();
	void onSelectionSave();
	void onSelectAllContacts();
	void onSelectOnlineContacts();
	void onSelectNotBusyContacts();
	void onSelectNoneContacts();
	void onReceiversContextMenuRequested(const QPoint &APos);
protected slots:
	void onDeleteDelayedItems();
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
private:
	QList<QStandardItem *> FDeleteDelayed;
	QMap<Jid, QStandardItem *> FStreamItems;
	QMap<Jid, QMap<QString, QStandardItem *> > FGroupItems;
	QMap<Jid, QMultiHash<Jid, QStandardItem *> > FContactItems;
};

#endif // RECEIVERSWIDGET_H
