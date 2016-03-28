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
	SupportedProxyModel(IServiceDiscovery *ADiscovery, IMessageWindow *AWindow, QObject *AParent) : QSortFilterProxyModel(AParent)
	{
		FWindow = AWindow;
		FDiscovery = ADiscovery;

		IMultiUserChatWindow *mucWindow = FWindow!=NULL ? qobject_cast<IMultiUserChatWindow *>(AWindow->instance()) : NULL;
		FMultiChat = mucWindow!=NULL ? mucWindow->multiUserChat() : NULL;

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
				if (FDiscovery->checkDiscoFeature(streamJid,contactJid,NS_MUC))
				{
					if (FMultiChat != NULL)
						return !FMultiChat->isUserPresent(contactJid);
					else if (FWindow)
						return FWindow->streamJid().pBare()!=contactJid.pBare() && FWindow->contactJid().pBare()!=contactJid.pBare();
				}
			}
			return false;
		}
		return QSortFilterProxyModel::filterAcceptsRow(AModelRow,AModelParent);
	}
private:
	IMessageWindow *FWindow;
	IMultiUserChat *FMultiChat;
	IServiceDiscovery *FDiscovery;
};


InviteUsersWidget::InviteUsersWidget(IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FReceiversWidget = NULL;

	IMessageWidgets *messageWidgets = PluginHelper::pluginInstance<IMessageWidgets>();
	if (messageWidgets)
	{
		FReceiversWidget = messageWidgets->newReceiversWidget(AWindow,ui.wdtReceivers);

		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery)
		{
			SupportedProxyModel *proxy = new SupportedProxyModel(discovery, AWindow, this);
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
