#include "exchangeapprovedialog.h"

#include <QTableWidgetItem>

enum ItemsTableColumns {
	COL_ACTION,
	COL_COUNT
};

ExchangeApproveDialog::ExchangeApproveDialog(IRoster *ARoster, const IRosterExchangeRequest &ARequest, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FRoster = ARoster;
	FRequest = ARequest;

	setWindowTitle(tr("Roster Modification - %1").arg(ARoster->streamJid().uBare()));
	setWindowIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTEREXCHANGE_REQUEST));

	ui.lblNotice->setText(tr("Contact '%1' offers you to make the following changes in your contact list:").arg(contactName(ARequest.contactJid)));

	ui.tbwItems->setWordWrap(true);
	ui.tbwItems->setTextElideMode(Qt::ElideNone);
	ui.tbwItems->setColumnCount(COL_COUNT);
	ui.tbwItems->setHorizontalHeaderLabels(QStringList()<<tr("Modification"));
	ui.tbwItems->horizontalHeader()->setResizeMode(COL_ACTION,QHeaderView::Stretch);

	ui.chbSubscribe->setChecked(true);
	ui.chbSubscribe->setVisible(false);

	connect(ui.btbButtons,SIGNAL(accepted()),SLOT(accept()));
	connect(ui.btbButtons,SIGNAL(rejected()),SLOT(reject()));

	connect(FRoster->xmppStream()->instance(),SIGNAL(aboutToClose()),SLOT(reject()));

	appendRequestItems(ARequest.items);
}

ExchangeApproveDialog::~ExchangeApproveDialog()
{
	emit dialogDestroyed();
}

bool ExchangeApproveDialog::subscribeNewContacts() const
{
	return ui.chbSubscribe->isChecked();
}

IRosterExchangeRequest ExchangeApproveDialog::receivedRequest() const
{
	IRosterExchangeRequest request = FRequest;
	request.streamJid = FRoster->streamJid();
	return request;
}

IRosterExchangeRequest ExchangeApproveDialog::approvedRequest() const
{
	IRosterExchangeRequest request = FRequest;
	request.streamJid = FRoster->streamJid();

	request.items.clear();
	for (QMap<QTableWidgetItem *, IRosterExchangeItem>::const_iterator it=FItems.constBegin(); it!=FItems.constEnd(); it++)
	{
		if (it.key()->checkState() == Qt::Checked)
			request.items.append(it.value());
	}

	return request;
}

QString ExchangeApproveDialog::groupSetToString(const QSet<QString> AGroups) const
{
	QStringList groups;
	foreach(QString group, AGroups)
		groups.append("'"+group+"'");
	return groups.join(", ");
}

QString ExchangeApproveDialog::contactName(const Jid &AContactJid, bool AWithJid) const
{
	IRosterItem ritem = FRoster->rosterItem(AContactJid);
	QString name = !ritem.name.isEmpty() ? ritem.name : AContactJid.uBare();
	if (AWithJid && !ritem.name.isEmpty())
		name = QString("%1 <%2>").arg(name,AContactJid.uBare());
	return name;
}

void ExchangeApproveDialog::appendRequestItems(const QList<IRosterExchangeItem> &AItems)
{
	for (QList<IRosterExchangeItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); it++)
	{
		QString actionText;
		IRosterItem ritem = FRoster->rosterItem(it->itemJid);
		if (it->action == ROSTEREXCHANGE_ACTION_ADD)
		{
			if (!ritem.isValid)
			{
				if (it->groups.isEmpty())
					actionText = tr("Add new contact '%1 <%2>'").arg(it->name,it->itemJid.uBare());
				else
					actionText = tr("Add new contact '%1 <%2>' to the group: %3").arg(it->name,it->itemJid.uBare(),groupSetToString(it->groups));
				ui.chbSubscribe->setVisible(true);
			}
			else if (!it->groups.isEmpty())
			{
				actionText = tr("Copy contact '%1' to the group: %2").arg(contactName(it->itemJid),groupSetToString(it->groups-ritem.groups));
			}
		}
		else if (ritem.isValid && it->action == ROSTEREXCHANGE_ACTION_DELETE)
		{
			QSet<QString> oldGroups = it->groups;
			oldGroups.intersect(ritem.groups);
			if (it->groups.isEmpty())
				actionText = tr("Remove contact '%1' from contact list").arg(contactName(it->itemJid));
			else
				actionText = tr("Remove contact '%1' from the group: %2").arg(contactName(it->itemJid),groupSetToString(oldGroups));
		}
		else if (ritem.isValid && it->action == ROSTEREXCHANGE_ACTION_MODIFY)
		{
			QSet<QString> newGroups = it->groups - ritem.groups;
			QSet<QString> oldGroups = ritem.groups - it->groups;
			if (it->name != ritem.name)
			{
				actionText = tr("Rename contact '%1' to '%2'").arg(contactName(it->itemJid),it->name);
			}
			if (!newGroups.isEmpty())
			{
				if (!actionText.isEmpty())
					actionText += "; ";
				actionText += tr("Copy contact '%1' to the group: %2").arg(contactName(it->itemJid),groupSetToString(newGroups));
			}
			if (!oldGroups.isEmpty())
			{
				if (!actionText.isEmpty())
					actionText += "; ";
				actionText += tr("Remove contact '%1' from the group: %2").arg(contactName(it->itemJid),groupSetToString(oldGroups));
			}
		}

		if (!actionText.isEmpty())
		{
			QTableWidgetItem *actionItem = new QTableWidgetItem;
			actionItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
			actionItem->setCheckState(Qt::Checked);
			actionItem->setText(actionText);

			ui.tbwItems->setRowCount(ui.tbwItems->rowCount()+1);
			ui.tbwItems->setItem(ui.tbwItems->rowCount()-1,COL_ACTION,actionItem);
			ui.tbwItems->verticalHeader()->setResizeMode(actionItem->row(),QHeaderView::ResizeToContents);

			FItems.insert(actionItem,*it);
		}
	}
}
