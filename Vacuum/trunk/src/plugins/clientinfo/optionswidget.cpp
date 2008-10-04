#include "optionswidget.h"

OptionsWidget::OptionsWidget(IClientInfo *AClientInfo)
{
  ui.setupUi(this);
  FClientInfo = AClientInfo;
  ui.chbAutoLoadSoftwareInfo->setCheckState( FClientInfo->checkOption(IClientInfo::AutoLoadSoftwareVersion) ? Qt::Checked : Qt::Unchecked );
}

OptionsWidget::~OptionsWidget()
{

}

void OptionsWidget::apply()
{
  FClientInfo->setOption(IClientInfo::AutoLoadSoftwareVersion,ui.chbAutoLoadSoftwareInfo->checkState() == Qt::Checked);
}