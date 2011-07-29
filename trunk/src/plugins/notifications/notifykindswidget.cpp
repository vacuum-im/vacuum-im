#include "notifykindswidget.h"

NotifyKindsWidget::NotifyKindsWidget(INotifications *ANotifications, const QString &AId, const QString &ATitle, ushort AKindMask, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.grbKinds->setTitle(ATitle);

	FNotifications = ANotifications;
	FNotificatorId = AId;

	ui.chbRoster->setEnabled(AKindMask & INotification::RosterNotify);
	ui.chbPopup->setEnabled(AKindMask & INotification::PopupWindow);
	ui.chbTray->setEnabled(AKindMask & INotification::TrayNotify);
	ui.chbSound->setEnabled(AKindMask & INotification::SoundPlay);
	ui.chbActivate->setEnabled(AKindMask & INotification::AutoActivate);

	connect(ui.chbRoster,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbPopup,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbTray,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbSound,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbActivate,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	reset();
}

NotifyKindsWidget::~NotifyKindsWidget()
{

}

void NotifyKindsWidget::apply()
{
	ushort kinds = FNotifications->notificationKinds(FNotificatorId);

	if (ui.chbRoster->isChecked())
		kinds |= INotification::RosterNotify;
	else
		kinds &= ~INotification::RosterNotify;

	if (ui.chbPopup->isChecked())
		kinds |= INotification::PopupWindow;
	else
		kinds &= ~INotification::PopupWindow;

	if (ui.chbTray->isChecked())
		kinds |= INotification::TrayNotify|INotification::TrayAction;
	else
		kinds &= ~(INotification::TrayNotify|INotification::TrayAction);

	if (ui.chbSound->isChecked())
		kinds |= INotification::SoundPlay;
	else
		kinds &= ~INotification::SoundPlay;

	if (ui.chbActivate->isChecked())
		kinds |= INotification::AutoActivate;
	else
		kinds &= ~INotification::AutoActivate;

	FNotifications->setNotificationKinds(FNotificatorId,kinds);
	emit childApply();
}

void NotifyKindsWidget::reset()
{
	ushort kinds = FNotifications->notificationKinds(FNotificatorId);
	ui.chbRoster->setChecked(kinds & INotification::RosterNotify);
	ui.chbPopup->setChecked(kinds & INotification::PopupWindow);
	ui.chbTray->setChecked(kinds & INotification::TrayNotify);
	ui.chbSound->setChecked(kinds & INotification::SoundPlay);
	ui.chbActivate->setChecked(kinds & INotification::AutoActivate);
	emit childReset();
}
