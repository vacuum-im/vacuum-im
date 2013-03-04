#ifndef RECEIVERSWIDGET_H
#define RECEIVERSWIDGET_H

#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersmodel.h>
#include "ui_receiverswidget.h"

class ReceiversWidget :
	public QWidget,
	public IMessageReceiversWidget
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
	virtual QList<Jid> receivers() const;
	virtual QString receiverName(const Jid &AReceiver) const;
	virtual void addReceiversGroup(const QString &AGroup);
	virtual void removeReceiversGroup(const QString &AGroup);
	virtual void addReceiver(const Jid &AReceiver);
	virtual void removeReceiver(const Jid &AReceiver);
	virtual void clear();
signals:
	void receiverAdded(const Jid &AReceiver);
	void receiverRemoved(const Jid &AReceiver);
protected:
	void initialize();
	QTreeWidgetItem *getReceiversGroup(const QString &AGroup);
	QTreeWidgetItem *getReceiver(const Jid &AReceiver, const QString &AName, QTreeWidgetItem *AParent);
	void createRosterTree();
protected slots:
	void onReceiversItemChanged(QTreeWidgetItem *AItem, int AColumn);
	void onSelectAllClicked();
	void onSelectAllOnlineClicked();
	void onSelectNoneClicked();
	void onAddClicked();
	void onUpdateClicked();
	void onAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	Ui::ReceiversWidgetClass ui;
private:
	IRoster *FRoster;
	IPresence *FPresence;
	IStatusIcons *FStatusIcons;
	IRostersModel *FRostersModel;
	IMessageWidgets *FMessageWidgets;
private:
	QList<Jid> FReceivers;
	IMessageWindow *FWindow;
	QHash<QString,QTreeWidgetItem *> FGroupItems;
	QMultiHash<Jid,QTreeWidgetItem *> FContactItems;
};

#endif // RECEIVERSWIDGET_H
