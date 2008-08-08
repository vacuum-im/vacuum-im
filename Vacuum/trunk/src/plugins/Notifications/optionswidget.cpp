#include "optionswidget.h"

OptionsWidget::OptionsWidget(INotifications *ANotifications, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FNotifications = ANotifications;
  updateWidget();
}

OptionsWidget::~OptionsWidget()
{

}

void OptionsWidget::applyOptions()
{
  FNotifications->setOption(INotifications::EnableRosterIcons,ui.chbEnableRosterIcons->isChecked());
  FNotifications->setOption(INotifications::EnablePopupWindows,ui.chbEnablePopupWindows->isChecked());
  FNotifications->setOption(INotifications::EnableTrayIcons,ui.chbEnableTrayIcons->isChecked());
  FNotifications->setOption(INotifications::EnableTrayActions,ui.chbEnableTrayActions->isChecked());
  FNotifications->setOption(INotifications::EnableSounds,ui.chbEnableSounds->isChecked());
}

void OptionsWidget::updateWidget()
{
  ui.chbEnableRosterIcons->setChecked(FNotifications->checkOption(INotifications::EnableRosterIcons));
  ui.chbEnablePopupWindows->setChecked(FNotifications->checkOption(INotifications::EnablePopupWindows));
  ui.chbEnableTrayIcons->setChecked(FNotifications->checkOption(INotifications::EnableTrayIcons));
  ui.chbEnableTrayActions->setChecked(FNotifications->checkOption(INotifications::EnableTrayActions));
  ui.chbEnableSounds->setChecked(FNotifications->checkOption(INotifications::EnableSounds));
}