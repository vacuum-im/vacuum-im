#include "optionscontainer.h"

#include <QVBoxLayout>

OptionsContainer::OptionsContainer(const IOptionsManager *AOptionsManager, QWidget *AParent) : QWidget(AParent)
{
	FOptionsManager = AOptionsManager;
}

OptionsContainer::~OptionsContainer()
{

}

IOptionsWidget *OptionsContainer::appendChild(const OptionsNode &ANode, const QString &ACaption)
{
	IOptionsWidget *widget = FOptionsManager->optionsNodeWidget(ANode,ACaption,this);
	if (layout() == NULL)
	{
		setLayout(new QVBoxLayout);
		layout()->setMargin(0);
	}
	layout()->addWidget(widget->instance());
	registerChild(widget);
	return widget;
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
