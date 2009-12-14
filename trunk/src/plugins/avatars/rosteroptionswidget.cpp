#include "rosteroptionswidget.h"

RosterOptionsWidget::RosterOptionsWidget(IAvatars *AAvatars, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FAvatars = AAvatars;
  ui.chbShowAvatars->setChecked(FAvatars->avatarsVisible());
}

RosterOptionsWidget::~RosterOptionsWidget()
{

}

void RosterOptionsWidget::apply()
{
  FAvatars->setAvatarsVisible(ui.chbShowAvatars->isChecked());
  emit optionsAccepted();
}
