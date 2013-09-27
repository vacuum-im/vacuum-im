#include "boxwidget.h"

BoxWidget::BoxWidget(QWidget *AParent, QBoxLayout::Direction ADirection) : QFrame(AParent)
{
	setFrameShape(QFrame::NoFrame);
	FLayout = new QBoxLayout(ADirection,this);
	FLayout->setMargin(0);
}

BoxWidget::~BoxWidget()
{

}

bool BoxWidget::isEmpty() const
{
	return FWidgetOrders.isEmpty();
}

QList<QWidget *> BoxWidget::widgets() const
{
	return FWidgetOrders.values();
}

int BoxWidget::widgetOrder(QWidget *AWidget) const
{
	return FWidgetOrders.key(AWidget);
}

QWidget *BoxWidget::widgetByOrder(int AOrderId) const
{
	return FWidgetOrders.value(AOrderId);
}

void BoxWidget::insertWidget(int AOrderId, QWidget *AWidget, int AStretch)
{
	if (!FWidgetOrders.contains(AOrderId))
	{
		removeWidget(AWidget);
		QMap<int, QWidget *>::const_iterator it = FWidgetOrders.lowerBound(AOrderId);
		int index = it!=FWidgetOrders.constEnd() ? FLayout->indexOf(it.value()) : -1;
		FLayout->insertWidget(index,AWidget,AStretch);
		FWidgetOrders.insert(AOrderId,AWidget);
		connect(AWidget,SIGNAL(destroyed(QObject *)),this,SLOT(onWidgetDestroyed(QObject *)));
		emit widgetInserted(AOrderId,AWidget);
	}
}

void BoxWidget::removeWidget(QWidget *AWidget)
{
	if (widgets().contains(AWidget))
	{
		FLayout->removeWidget(AWidget);
		AWidget->setParent(NULL);
		FWidgetOrders.remove(widgetOrder(AWidget));
		disconnect(AWidget,SIGNAL(destroyed(QObject *)),this,SLOT(onWidgetDestroyed(QObject *)));
		emit widgetRemoved(AWidget);
	}
}

int BoxWidget::spacing() const
{
	return FLayout->spacing();
}

void BoxWidget::setSpacing(int ASpace)
{
	FLayout->setSpacing(ASpace);
}

int BoxWidget::stretch(QWidget *AWidget) const
{
	return FLayout->stretch(FLayout->indexOf(AWidget));
}

void BoxWidget::setStretch(QWidget *AWidget, int AStretch)
{
	FLayout->setStretch(FLayout->indexOf(AWidget),AStretch);
}

QBoxLayout::Direction BoxWidget::direction() const
{
	return FLayout->direction();
}

void BoxWidget::setDirection(QBoxLayout::Direction ADirection)
{
	FLayout->setDirection(ADirection);
}

void BoxWidget::onWidgetDestroyed(QObject *AObject)
{
	for (QMap<int,QWidget *>::iterator it=FWidgetOrders.begin(); it!=FWidgetOrders.end(); )
	{
		if (it.value() == AObject)
		{
			QWidget *widget = it.value();
			FWidgetOrders.erase(it);
			emit widgetRemoved(widget);
			break;
		}
		else
		{
			++it;
		}
	}
}
