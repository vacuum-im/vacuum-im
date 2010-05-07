#include "statewidget.h"

#include <QDateTime>

#define ADR_PERMIT_STATUS     Action::DR_Parametr1

StateWidget::StateWidget(IChatStates *AChatStates, IChatWindow *AWindow, QWidget *AParent) : QToolButton(AParent)
{
	FWindow = AWindow;
	FChatStates = AChatStates;

	FMenu = new Menu(this);

	Action *action = new Action(FMenu);
	action->setCheckable(true);
	action->setText(tr("Default"));
	action->setData(ADR_PERMIT_STATUS, IChatStates::StatusDefault);
	connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(action);

	action = new Action(FMenu);
	action->setCheckable(true);
	action->setText(tr("Always send"));
	action->setData(ADR_PERMIT_STATUS, IChatStates::StatusEnable);
	connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(action);

	action = new Action(FMenu);
	action->setCheckable(true);
	action->setText(tr("Never send"));
	action->setData(ADR_PERMIT_STATUS, IChatStates::StatusDisable);
	connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
	FMenu->addAction(action);

	setMenu(FMenu);
	setToolTip(tr("User chat status"));

	connect(FChatStates->instance(),SIGNAL(permitStatusChanged(const Jid &, int)),
	        SLOT(onPermitStatusChanged(const Jid &, int)));
	connect(FChatStates->instance(),SIGNAL(userChatStateChanged(const Jid &, const Jid &, int)),
	        SLOT(onUserChatStateChanged(const Jid &, const Jid &, int)));

	onPermitStatusChanged(FWindow->contactJid(),FChatStates->permitStatus(FWindow->contactJid()));
	onUserChatStateChanged(FWindow->streamJid(),FWindow->contactJid(),FChatStates->userChatState(FWindow->streamJid(),FWindow->contactJid()));
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
	if (FWindow->contactJid() && AContactJid)
	{
		foreach(Action *action, FMenu->groupActions(AG_DEFAULT))
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
			state = tr("Unknown");
			iconKey = MNI_CHATSTATES_UNKNOWN;
		}

		setText(state);
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,iconKey);
	}
}
