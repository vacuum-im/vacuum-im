#include "inviteuserswidget.h"

#include <QVBoxLayout>
#include <definitions/namespaces.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <utils/pluginhelper.h>

class SupportedProxyModel :
	public QSortFilterProxyModel
{
public:
	SupportedProxyModel(IServiceDiscovery *ADiscovery, IMultiUserChat *AMultiChat, QObject *AParent) : QSortFilterProxyModel(AParent)
	{
		FMultiChat = AMultiChat;
		FDiscovery = ADiscovery;
		setDynamicSortFilter(true);
	}
protected:
	bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
	{
		QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);
		if (index.data(RDR_KIND).toInt() == RIK_CONTACT)
		{
			Jid streamJid = index.data(RDR_STREAM_JID).toString();
			foreach(const Jid &contactJid, index.data(RDR_RESOURCES).toStringList())
			{
				if (FDiscovery->checkDiscoFeature(streamJid,contactJid,QString::null,NS_MUC,false))
				{
					if (FMultiChat==NULL || !FMultiChat->isUserPresent(contactJid))
						return true;
				}
			}
			return false;
		}
		return QSortFilterProxyModel::filterAcceptsRow(AModelRow,AModelParent);
	}
private:
	IMultiUserChat *FMultiChat;
	IServiceDiscovery *FDiscovery;
};


InviteUsersWidget::InviteUsersWidget(IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FWindow = AWindow;
	FReceiversWidget = NULL;

	IMessageWidgets *messageWidgets = PluginHelper::pluginInstance<IMessageWidgets>();
	if (messageWidgets)
	{
		FReceiversWidget = messageWidgets->newReceiversWidget(AWindow,ui.wdtReceivers);

		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery)
		{
			IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(AWindow->instance());
			IMultiUserChat *multiChat = chatWindow!=NULL ? chatWindow->multiUserChat() : NULL;

			SupportedProxyModel *proxy = new SupportedProxyModel(discovery, multiChat, this);
			FReceiversWidget->insertProxyModel(proxy);
		}

		ui.wdtReceivers->setLayout(new QVBoxLayout());
		ui.wdtReceivers->layout()->addWidget(FReceiversWidget->instance());
		ui.wdtReceivers->layout()->setMargin(0);
	}

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogButtonsAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(onDialogButtonsRejected()));
}

InviteUsersWidget::~InviteUsersWidget()
{

}

QSize InviteUsersWidget::sizeHint() const
{
	return QWidget::sizeHint().expandedTo(QSize(240,400));
}

void InviteUsersWidget::onDialogButtonsAccepted()
{
	emit inviteAccepted(FReceiversWidget->selectedAddresses());
}

void InviteUsersWidget::onDialogButtonsRejected()
{
	emit inviteRejected();
}
