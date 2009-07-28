#include "rosteroptionswidget.h"

RosterOptionsWidget::RosterOptionsWidget(IAvatars *AAvatars, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FAvatars = AAvatars;
  ui.chbShowAvatars->setChecked(FAvatars->checkOption(IAvatars::ShowAvatars));
  ui.chbAlignLeftAvatars->setChecked(FAvatars->checkOption(IAvatars::AvatarsAlignLeft));
}

RosterOptionsWidget::~RosterOptionsWidget()
{

}

void RosterOptionsWidget::apply()
{
  FAvatars->setOption(IAvatars::ShowAvatars,ui.chbShowAvatars->isChecked());
  FAvatars->setOption(IAvatars::AvatarsAlignLeft,ui.chbAlignLeftAvatars->isChecked());
  emit optionsAccepted();
}
