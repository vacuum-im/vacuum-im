#include "usercontextmenu.h"

UserContextMenu::UserContextMenu(IRostersModel *AModel, IRostersView *AView, IMessageChatWindow *AWindow) : Menu(AWindow->instance())
{
	FRosterIndex = NULL;
	FRostersModel = AModel;
	FRostersView = AView;
	FChatWindow = AWindow;

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
	connect(this,SIGNAL(aboutToHide()),SLOT(onAboutToHide()));
	connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
	connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *,int)),SLOT(onRosterIndexDataChanged(IRosterIndex *,int)));
	connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRosterIndexDestroyed(IRosterIndex *)));
	connect(FChatWindow->address()->instance(),SIGNAL(addressChanged(const Jid &,const Jid &)),SLOT(onChatWindowAddressChanged(const Jid &,const Jid &)));

	onRosterIndexDestroyed(FRosterIndex);
}

UserContextMenu::~UserContextMenu()
{

}

bool UserContextMenu::isAcceptedIndex(IRosterIndex *AIndex)
{
	if (AIndex!=NULL && FChatWindow->streamJid()==AIndex->data(RDR_STREAM_JID).toString())
	{
		Jid indexJid = AIndex->data(RDR_PREP_FULL_JID).toString();
		if (FChatWindow->contactJid() == indexJid)
			return true;
		if (indexJid.resource().isEmpty() && (FChatWindow->contactJid() && indexJid))
			return true;
	}
	return false;
}

void UserContextMenu::updateMenu()
{
	if (FRosterIndex)
	{
		QString name = FRosterIndex->data(RDR_NAME).toString();
		if (name.isEmpty())
			name = FChatWindow->contactJid().uBare();

		Jid jid = FRosterIndex->data(RDR_PREP_FULL_JID).toString();
		if (!jid.resource().isEmpty())
			name += "/" + jid.resource();

		setTitle(name);
		menuAction()->setVisible(true);
	}
	else
	{
		setTitle(FChatWindow->contactJid().uFull());
		menuAction()->setVisible(false);
	}
}

void UserContextMenu::onAboutToShow()
{
	if (FRosterIndex)
	{
		QList<IRosterIndex *> indexes;
		indexes.append(FRosterIndex);
		FRostersView->contextMenuForIndex(indexes,NULL,this);
	}
}

void UserContextMenu::onAboutToHide()
{
	clear();
}

void UserContextMenu::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (FRosterIndex==NULL && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void UserContextMenu::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == FRosterIndex)
	{
		if (ARole == RDR_PREP_FULL_JID)
		{
			if (isAcceptedIndex(AIndex))
				updateMenu();
			else
				onRosterIndexDestroyed(FRosterIndex);
		}
		else if (ARole == RDR_NAME)
		{
			updateMenu();
		}
	}
	else if (FRosterIndex==NULL && ARole==RDR_PREP_FULL_JID && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void UserContextMenu::onRosterIndexDestroyed(IRosterIndex *AIndex)
{
	if (FRosterIndex == AIndex)
	{
		FRosterIndex = FRostersModel->getContactIndexList(FChatWindow->streamJid(),FChatWindow->contactJid()).value(0);
		updateMenu();
	}
}

void UserContextMenu::onChatWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	onRosterIndexDestroyed(FRosterIndex);
}
