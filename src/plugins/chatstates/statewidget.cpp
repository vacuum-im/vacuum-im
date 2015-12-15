#include "statewidget.h"

#include <QDateTime>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <utils/textmanager.h>
#include <utils/iconstorage.h>
#include <utils/menu.h>

#define ADR_PERMIT_STATUS     Action::DR_Parametr1

StateWidget::StateWidget(IChatStates *AChatStates, IMessageWindow *AWindow, QWidget *AParent) : QToolButton(AParent)
{
	FWindow = AWindow;
	FChatStates = AChatStates;
	FMultiWindow = qobject_cast<IMultiUserChatWindow *>(AWindow->instance());

	FMenu = new Menu(this);
	setMenu(FMenu);

	Action *permitDefault = new Action(FMenu);
	permitDefault->setCheckable(true);
	permitDefault->setText(tr("Default"));
	permitDefault->setData(ADR_PERMIT_STATUS, IChatStates::StatusDefault);
	connect(permitDefault,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(permitDefault);

	Action *permitEnable = new Action(FMenu);
	permitEnable->setCheckable(true);
	permitEnable->setText(tr("Always send my chat activity"));
	permitEnable->setData(ADR_PERMIT_STATUS, IChatStates::StatusEnable);
	connect(permitEnable,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(permitEnable);

	Action *permitDisable = new Action(FMenu);
	permitDisable->setCheckable(true);
	permitDisable->setText(tr("Never send my chat activity"));
	permitDisable->setData(ADR_PERMIT_STATUS, IChatStates::StatusDisable);
	connect(permitDisable,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(permitDisable);

	connect(FChatStates->instance(),SIGNAL(permitStatusChanged(const Jid &, int)),SLOT(onPermitStatusChanged(const Jid &, int)));
	connect(FWindow->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onWindowAddressChanged(const Jid &, const Jid &)));

	if (FMultiWindow == NULL)
	{
		setToolTip(tr("User activity in chat"));
		connect(FChatStates->instance(),SIGNAL(userChatStateChanged(const Jid &, const Jid &, int)),SLOT(onUserChatStateChanged(const Jid &, const Jid &, int)));
	}
	else
	{
		setToolTip(tr("Participants activity in conference"));
		connect(FChatStates->instance(),SIGNAL(userRoomStateChanged(const Jid &, const Jid &, int)),SLOT(onUserRoomStateChanged(const Jid &, const Jid &, int)));
	}

	onWindowAddressChanged(FWindow->streamJid(),FWindow->contactJid());
}

StateWidget::~StateWidget()
{

}

void StateWidget::onStatusActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		FChatStates->setPermitStatus(FWindow->contactJid(),action->data(ADR_PERMIT_STATUS).toInt());
		action->setChecked(true);
	}
}

void StateWidget::onPermitStatusChanged(const Jid &AContactJid, int AStatus)
{
	if (FWindow->contactJid().pBare() == AContactJid.pBare())
	{
		foreach(Action *action, FMenu->actions(AG_DEFAULT))
			action->setChecked(action->data(ADR_PERMIT_STATUS).toInt()==AStatus);
	}
}

void StateWidget::onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState)
{
	if (FWindow->streamJid()==AStreamJid && FWindow->contactJid()==AContactJid)
	{
		QString state;
		QString iconKey;

		if (AState == IChatStates::StateActive)
		{
			state = tr("Active");
			iconKey = MNI_CHATSTATES_ACTIVE;
		}
		else if (AState == IChatStates::StateComposing)
		{
			state = tr("Composing");
			iconKey = MNI_CHATSTATES_COMPOSING;
		}
		else if (AState == IChatStates::StatePaused)
		{
			state = tr("Paused");
			iconKey = MNI_CHATSTATES_PAUSED;
		}
		else if (AState == IChatStates::StateInactive)
		{
			state = tr("Inactive %1").arg(QDateTime::currentDateTime().toString("hh:mm"));
			iconKey = MNI_CHATSTATES_INACTIVE;
		}
		else if (AState == IChatStates::StateGone)
		{
			state = tr("Gone %1").arg(QDateTime::currentDateTime().toString("hh:mm"));
			iconKey = MNI_CHATSTATES_GONE;
		}
		else
		{
			iconKey = MNI_CHATSTATES_UNKNOWN;
		}

		setText(state);
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,iconKey);
	}
}

void StateWidget::onUserRoomStateChanged(const Jid &AStreamJid, const Jid &AUserJid, int AState)
{
	if (FWindow->streamJid()==AStreamJid && FWindow->contactJid().pBare()==AUserJid.pBare())
	{
		QString state;
		QString iconKey;

		IMultiUser *user = FMultiWindow->multiUserChat()->findUser(AUserJid.resource());
		if (FMultiWindow->multiUserChat()->mainUser() != user)
		{
			if (AState == IChatStates::StateActive)
				FActive += AUserJid;
			else
				FActive -= AUserJid;

			if (AState == IChatStates::StateComposing)
				FComposing += AUserJid;
			else
				FComposing -= AUserJid;

			if (AState == IChatStates::StatePaused)
				FPaused += AUserJid;
			else
				FPaused -= AUserJid;
		}

		if (!FComposing.isEmpty())
		{
			int hidden = 0;
			foreach(const Jid &userJid, FComposing)
			{
				QString nick = TextManager::getElidedString(userJid.resource(),Qt::ElideRight,10);
				if (state.isEmpty())
					state = nick;
				else if (state.length() < 20)
					state += QString(", %1").arg(nick);
				else
					hidden++;
			}
			if (hidden > 0)
			{
				state += " ";
				state += tr("and %1 other").arg(hidden);
			}
			iconKey = MNI_CHATSTATES_COMPOSING;
		}
		else
		{
			iconKey = MNI_CHATSTATES_UNKNOWN;
		}

		setText(state);
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,iconKey);
	}
}

void StateWidget::onWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	if (FMultiWindow == NULL)
		onUserChatStateChanged(FWindow->streamJid(),FWindow->contactJid(),FChatStates->userChatState(FWindow->streamJid(),FWindow->contactJid()));
	else
		onUserRoomStateChanged(FWindow->streamJid(),FWindow->contactJid(),IChatStates::StateUnknown);
	onPermitStatusChanged(FWindow->contactJid(),FChatStates->permitStatus(FWindow->contactJid()));
}
