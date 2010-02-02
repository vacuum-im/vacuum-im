#include "optionswidget.h"

OptionsWidget::OptionsWidget(INotifications *ANotifications, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FNotifications = ANotifications;

  ui.chbEnableRosterIcons->setChecked(FNotifications->checkOption(INotifications::EnableRosterIcons));
  ui.chbEnablePopupWindows->setChecked(FNotifications->checkOption(INotifications::EnablePopupWindows));
  ui.chbEnableTrayIcons->setChecked(FNotifications->checkOption(INotifications::EnableTrayIcons));
  ui.chbEnableSounds->setChecked(FNotifications->checkOption(INotifications::EnableSounds));
  ui.chbEnableAutoActivate->setChecked(FNotifications->checkOption(INotifications::EnableAutoActivate));
}

OptionsWidget::~OptionsWidget()
{

}

void OptionsWidget::appendKindsWidget(NotifyKindsWidget *AWidget)
{
  layout()->addWidget(AWidget);
  FKindsWidgets.append(AWidget);
}

void OptionsWidget::apply()
{
  foreach(NotifyKindsWidget *widget, FKindsWidgets)
    widget->apply();

  FNotifications->setOption(INotifications::EnableRosterIcons,ui.chbEnableRosterIcons->isChecked());
  FNotifications->setOption(INotifications::EnablePopupWindows,ui.chbEnablePopupWindows->isChecked());
  FNotifications->setOption(INotifications::EnableTrayIcons,ui.chbEnableTrayIcons->isChecked());
  FNotifications->setOption(INotifications::EnableSounds,ui.chbEnableSounds->isChecked());
  FNotifications->setOption(INotifications::EnableAutoActivate,ui.chbEnableAutoActivate->isChecked());

  emit optionsAccepted();
}

