#include "optionswidget.h"

OptionsWidget::OptionsWidget(INotifications *ANotifications, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FNotifications = ANotifications;

	connect(ui.chbEnableRosterIcons,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnablePopupWindows,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnableTrayIcons,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnableSounds,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnableAutoActivate,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnableAlerts,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbExpandRosterGroups,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbDisableSoundsWhenDND,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.spbPopupTimeout,SIGNAL(valueChanged(int)),SIGNAL(modified()));

	reset();
}

OptionsWidget::~OptionsWidget()
{

}

void OptionsWidget::apply()
{
	Options::node(OPV_NOTIFICATIONS_ROSTERNOTIFY).setValue(ui.chbEnableRosterIcons->isChecked());
	Options::node(OPV_NOTIFICATIONS_POPUPWINDOW).setValue(ui.chbEnablePopupWindows->isChecked());
	Options::node(OPV_NOTIFICATIONS_TRAYNOTIFY).setValue(ui.chbEnableTrayIcons->isChecked());
	Options::node(OPV_NOTIFICATIONS_SOUND).setValue(ui.chbEnableSounds->isChecked());
	Options::node(OPV_NOTIFICATIONS_AUTOACTIVATE).setValue(ui.chbEnableAutoActivate->isChecked());
	Options::node(OPV_NOTIFICATIONS_ENABLEALERTS).setValue(ui.chbEnableAlerts->isChecked());
	Options::node(OPV_NOTIFICATIONS_EXPANDGROUP).setValue(ui.chbExpandRosterGroups->isChecked());
	Options::node(OPV_NOTIFICATIONS_NOSOUNDIFDND).setValue(ui.chbDisableSoundsWhenDND->isChecked());
	Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).setValue(ui.spbPopupTimeout->value());
	emit childApply();
}

void OptionsWidget::reset()
{
	ui.chbEnableRosterIcons->setChecked(Options::node(OPV_NOTIFICATIONS_ROSTERNOTIFY).value().toBool());
	ui.chbEnablePopupWindows->setChecked(Options::node(OPV_NOTIFICATIONS_POPUPWINDOW).value().toBool());
	ui.chbEnableTrayIcons->setChecked(Options::node(OPV_NOTIFICATIONS_TRAYNOTIFY).value().toBool());
	ui.chbEnableSounds->setChecked(Options::node(OPV_NOTIFICATIONS_SOUND).value().toBool());
	ui.chbEnableAutoActivate->setChecked(Options::node(OPV_NOTIFICATIONS_AUTOACTIVATE).value().toBool());
	ui.chbEnableAlerts->setChecked(Options::node(OPV_NOTIFICATIONS_ENABLEALERTS).value().toBool());
	ui.chbExpandRosterGroups->setChecked(Options::node(OPV_NOTIFICATIONS_EXPANDGROUP).value().toBool());
	ui.chbDisableSoundsWhenDND->setChecked(Options::node(OPV_NOTIFICATIONS_NOSOUNDIFDND).value().toBool());
	ui.spbPopupTimeout->setValue(Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt());
	emit childReset();
}
