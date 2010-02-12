#include "miscoptionswidget.h"

MiscOptionsWidget::MiscOptionsWidget(IClientInfo *AClientInfo, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FClientInfo = AClientInfo;
  ui.chbshareOSVersion->setChecked(FClientInfo->shareOSVersion());
}

MiscOptionsWidget::~MiscOptionsWidget()
{

}

void MiscOptionsWidget::apply()
{
  FClientInfo->setShareOSVersion(ui.chbshareOSVersion->isChecked());
  emit optionsAccepted();
}
