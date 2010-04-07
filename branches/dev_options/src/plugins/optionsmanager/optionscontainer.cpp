#include "optionscontainer.h"

OptionsContainer::OptionsContainer(QWidget *AParent) : QWidget(AParent)
{

}

OptionsContainer::~OptionsContainer()
{

}

void OptionsContainer::registerChild(IOptionsWidget *AWidget)
{
  connect(this,SIGNAL(childApply()),AWidget->instance(),SLOT(apply()));
  connect(this,SIGNAL(childReset()),AWidget->instance(),SLOT(reset()));
  connect(AWidget->instance(),SIGNAL(modified()),this,SIGNAL(modified()));
}

void OptionsContainer::apply()
{
  emit childApply();
}

void OptionsContainer::reset()
{
  emit childReset();
}
