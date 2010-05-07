#include "notifykindswidget.h"

NotifyKindsWidget::NotifyKindsWidget(INotifications *ANotifications, const QString &AId, const QString &ATitle, uchar AKindMask, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.grbKinds->setTitle(ATitle);

	FNotifications = ANotifications;
	FNotificatorId = AId;

	ui.chbRoster->setEnabled(AKindMask & INotification::RosterIcon);
	ui.chbPopup->setEnabled(AKindMask & INotification::PopupWindow);
	ui.chbTray->setEnabled(AKindMask & INotification::TrayIcon);
	ui.chbSound->setEnabled(AKindMask & INotification::PlaySound);
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
	uchar kinds = 0;
	if (ui.chbRoster->isChecked())
		kinds |= INotification::RosterIcon;
	if (ui.chbPopup->isChecked())
		kinds |= INotification::PopupWindow;
	if (ui.chbTray->isChecked())
		kinds |= INotification::TrayIcon|INotification::TrayAction;
	if (ui.chbSound->isChecked())
		kinds |= INotification::PlaySound;
	if (ui.chbActivate->isChecked())
		kinds |= INotification::AutoActivate;
	FNotifications->setNotificatorKinds(FNotificatorId,kinds);
	emit childApply();
}

void NotifyKindsWidget::reset()
{
	uchar kinds = FNotifications->notificatorKinds(FNotificatorId);
	ui.chbRoster->setChecked(kinds & INotification::RosterIcon);
	ui.chbPopup->setChecked(kinds & INotification::PopupWindow);
	ui.chbTray->setChecked(kinds & INotification::TrayIcon);
	ui.chbSound->setChecked(kinds & INotification::PlaySound);
	ui.chbActivate->setChecked(kinds & INotification::AutoActivate);
	emit childReset();
}
