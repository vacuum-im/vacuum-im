#include "stateoptionswidget.h"

StateOptionsWidget::StateOptionsWidget(IChatStates *AChatStates, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FChatStates = AChatStates;

  ui.chbEnableChatStates->setChecked(FChatStates->isEnabled());
}

StateOptionsWidget::~StateOptionsWidget()
{

}

void StateOptionsWidget::apply()
{
  FChatStates->setEnabled(ui.chbEnableChatStates->isChecked());
  emit optionsApplied();
}
