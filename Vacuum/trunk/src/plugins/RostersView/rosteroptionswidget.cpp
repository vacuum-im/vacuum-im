#include "rosteroptionswidget.h"

RosterOptionsWidget::RosterOptionsWidget(IRostersViewPlugin *ARosterViewPlugin)
{
  ui.setupUi(this);
  FRostersViewPlugin = ARosterViewPlugin;

  ui.chbShowOfflineContacts->setChecked(FRostersViewPlugin->checkOption(IRostersView::ShowOfflineContacts) ? Qt::Checked : Qt::Unchecked);
  ui.chbShowOnlineFirst->setChecked(FRostersViewPlugin->checkOption(IRostersView::ShowOnlineFirst) ? Qt::Checked : Qt::Unchecked);
  ui.chbShowFooterText->setChecked(FRostersViewPlugin->checkOption(IRostersView::ShowFooterText) ? Qt::Checked : Qt::Unchecked);
  ui.chbShowResource->setChecked(FRostersViewPlugin->checkOption(IRostersView::ShowResource) ? Qt::Checked : Qt::Unchecked);
}

RosterOptionsWidget::~RosterOptionsWidget()
{

}

void RosterOptionsWidget::apply()
{
  FRostersViewPlugin->setOption(IRostersView::ShowOfflineContacts,ui.chbShowOfflineContacts->checkState() == Qt::Checked);
  FRostersViewPlugin->setOption(IRostersView::ShowOnlineFirst,ui.chbShowOnlineFirst->checkState() == Qt::Checked);
  FRostersViewPlugin->setOption(IRostersView::ShowFooterText,ui.chbShowFooterText->checkState() == Qt::Checked);
  FRostersViewPlugin->setOption(IRostersView::ShowResource,ui.chbShowResource->checkState() == Qt::Checked);
}