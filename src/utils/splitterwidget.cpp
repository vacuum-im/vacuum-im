#include "splitterwidget.h"

#include <QEvent>
#include <QVBoxLayout>
#include <QResizeEvent>

SplitterWidget::SplitterWidget(QWidget *AParent, Qt::Orientation AOrientation) : QFrame(AParent)
{
	setLayout(new QVBoxLayout);
	layout()->setMargin(0);

	FMovingWidgets = false;

	FSplitter = new QSplitter(AOrientation,this);
	FSplitter->installEventFilter(this);
	connect(FSplitter,SIGNAL(splitterMoved(int,int)),SLOT(onSplitterMoved(int,int)));
	layout()->addWidget(FSplitter);

	FHandleWidgets.insert(0,NULL);

	setHandlesCollapsible(false);
}

SplitterWidget::~SplitterWidget()
{

}

QList<QWidget *> SplitterWidget::widgets() const
{
	return FWidgetOrders.values();
}

int SplitterWidget::widgetHandle(QWidget *AWidget) const
{
	return FWidgetOrders.values().contains(AWidget) ? findWidgetHandle(widgetOrder(AWidget)) : -1;
}

int SplitterWidget::widgetOrder(QWidget *AWidget) const
{
	return FWidgetOrders.key(AWidget);
}

QWidget *SplitterWidget::widgetByOrder(int AOrderId) const
{
	return FWidgetOrders.value(AOrderId);
}

void SplitterWidget::insertWidget(int AOrderId, QWidget *AWidget, int AStretch, int AHandle)
{
	if (!FWidgetOrders.contains(AOrderId))
	{
		removeWidget(AWidget);

		FWidgetHandles.insert(AWidget,AHandle);
		BoxWidget *boxWidget = getHandleBox(findWidgetHandle(AOrderId));
		boxWidget->insertWidget(AOrderId,AWidget,AStretch);
		updateStretchFactor();
	}
}

void SplitterWidget::removeWidget(QWidget *AWidget)
{
	for (QMap<int,BoxWidget *>::iterator it = FHandleWidgets.begin(); it!=FHandleWidgets.end(); ++it)
	{
		BoxWidget *&boxWidget = it.value();
		if (boxWidget!=NULL && boxWidget->widgets().contains(AWidget))
		{
			boxWidget->removeWidget(AWidget);
			FWidgetHandles.remove(AWidget);
			updateStretchFactor();
			break;
		}
	}
}

QList<int> SplitterWidget::handles() const
{
	return FHandleWidgets.keys();
}

void SplitterWidget::insertHandle(int AOrderId)
{
	if (AOrderId>0 && !FHandleWidgets.contains(AOrderId))
	{
		FHandleWidgets.insert(AOrderId,NULL);

		BoxWidget *srcBoxWidget = NULL;
		for (QMap<int,BoxWidget *>::const_iterator it = FHandleWidgets.constBegin(); it!=FHandleWidgets.constEnd() && it.key()<AOrderId; ++it)
			srcBoxWidget = it.value();

		if (srcBoxWidget)
		{
			int srcIndex = handleIndexByOrder(FHandleWidgets.key(srcBoxWidget));
			int srcSize = FSplitter->sizes().value(srcIndex);

			FMovingWidgets = true;
			BoxWidget *dstBoxWidget = NULL;
			foreach(QWidget *widget, srcBoxWidget->widgets())
			{
				int order = srcBoxWidget->widgetOrder(widget);
				if (order >= AOrderId)
				{
					int stretch = srcBoxWidget->stretch(widget);
					srcBoxWidget->removeWidget(widget);

					if (dstBoxWidget == NULL)
						dstBoxWidget = getHandleBox(AOrderId);
					dstBoxWidget->insertWidget(order,widget,stretch);
				}
			}
			FMovingWidgets = false;
			updateStretchFactor();

			if (dstBoxWidget)
			{
				QList<int> newSizes = FSplitter->sizes();
				int srcStretch = (orientation()==Qt::Vertical ? srcBoxWidget->sizePolicy().verticalStretch() : srcBoxWidget->sizePolicy().horizontalStretch())+1;
				int dstStretch = (orientation()==Qt::Vertical ? dstBoxWidget->sizePolicy().verticalStretch() : dstBoxWidget->sizePolicy().horizontalStretch())+1;
				newSizes[srcIndex] = qRound(srcSize * double(srcStretch)/(srcStretch+dstStretch));
				newSizes[srcIndex+1] = qRound(srcSize * double(dstStretch)/(srcStretch+dstStretch));
				FSplitter->setSizes(newSizes);
			}
		}
	}
}

void SplitterWidget::removeHandle(int AOrderId)
{
	if (AOrderId>0 && FHandleWidgets.contains(AOrderId))
	{
		FHandleSizes.remove(AOrderId);
		BoxWidget *srcBoxWidget = FHandleWidgets.take(AOrderId);
		if (srcBoxWidget && !srcBoxWidget->isEmpty())
		{
			BoxWidget *dstBoxWidget = getHandleBox(findWidgetHandle(srcBoxWidget->widgetOrder(srcBoxWidget->widgets().value(0))));
			if (dstBoxWidget != srcBoxWidget)
			{
				QList<int> newSizes = FSplitter->sizes();

				FMovingWidgets = true;
				foreach(QWidget *widget, srcBoxWidget->widgets())
				{
					int order = srcBoxWidget->widgetOrder(widget);
					int stretch = srcBoxWidget->stretch(widget);
					srcBoxWidget->removeWidget(widget);
					dstBoxWidget->insertWidget(order,widget,stretch);
				}
				FMovingWidgets = false;
				updateStretchFactor();

				int srcIndex = handleIndexByOrder(FHandleWidgets.key(srcBoxWidget));
				int dstIndex = handleIndexByOrder(FHandleWidgets.key(dstBoxWidget));
				newSizes[dstIndex] += newSizes[srcIndex];
				newSizes[srcIndex] = 0;
				FSplitter->setSizes(newSizes);
			}
		}
	}
}

int SplitterWidget::handleWidth() const
{
	return FSplitter->handleWidth();
}

void SplitterWidget::setHandleWidth(int AWidth)
{
	FSplitter->setHandleWidth(AWidth);
}

int SplitterWidget::handleSize(int AOrderId) const
{
	BoxWidget *boxWidget = FHandleWidgets.value(AOrderId);
	if (boxWidget && !boxWidget->isEmpty())
	{
		int index = handleIndexByOrder(AOrderId);
		if (index >= 0)
			return FSplitter->sizes().value(index);
	}
	return 0;
}

void SplitterWidget::setHandleSize(int AOrderId, int ASize)
{
	BoxWidget *boxWidget = FHandleWidgets.value(AOrderId);
	if (ASize>=0 && FHandleWidgets.count()>1 && boxWidget && !boxWidget->isEmpty())
	{
		int index = handleIndexByOrder(AOrderId);
		if (index >= 0)
		{
			QList<int> newSizes = FSplitter->sizes();
			int &curSize = newSizes[index];
			if (index > 0)
				newSizes[index-1] += curSize - ASize;
			else
				newSizes[index+1] += curSize - ASize;
			curSize = ASize;
			FSplitter->setSizes(newSizes);
			saveHandleFixedSizes();
		}
	}
}

QList<int> SplitterWidget::handleSizes() const
{
	QList<int> sizes;
	QList<int> splitterSizes = FSplitter->sizes();
	for (QMap<int,BoxWidget *>::const_iterator it=FHandleWidgets.constBegin(); it!=FHandleWidgets.constEnd(); ++it)
	{
		int index = handleIndexByOrder(it.key());
		if (index >= 0)
			sizes.append(splitterSizes.at(index));
		else
			sizes.append(0);
	}
	return sizes;
}

void SplitterWidget::setHandleSizes(const QList<int> &ASizes)
{
	if (FHandleWidgets.count() == ASizes.count())
	{
		QList<int> splitterSizes;
		QList<BoxWidget *> handleWidgets = FHandleWidgets.values();
		for (int index=0; index<FSplitter->count(); index++)
		{
			int sizesIndex = handleWidgets.indexOf(qobject_cast<BoxWidget *>(FSplitter->widget(index)));
			if (sizesIndex >= 0)
				splitterSizes.append(ASizes.value(sizesIndex));
			else
				splitterSizes.append(0);
		}
		FSplitter->setSizes(splitterSizes);
		saveHandleFixedSizes();
	}
}

bool SplitterWidget::isHandlesCollaplible() const
{
	return FSplitter->childrenCollapsible();
}

void SplitterWidget::setHandlesCollapsible(bool ACollapsible)
{
	FSplitter->setChildrenCollapsible(ACollapsible);
}

void SplitterWidget::getHandleRange(int AOrderId, int *AMin, int *AMax) const
{
	if (AMin)
		*AMin = 0;
	if (AMax)
		*AMax = 0;

	int index = handleIndexByOrder(AOrderId);
	if (index >= 0)
		FSplitter->getRange(index,AMin,AMax);
}

bool SplitterWidget::isHandleCollapsible(int AOrderId) const
{
	int index = handleIndexByOrder(AOrderId);
	return index>=0 ? FSplitter->isCollapsible(index) : isHandlesCollaplible();
}

void SplitterWidget::setHandleCollapsible(int AOrderId, bool ACollapsible)
{
	int index = handleIndexByOrder(AOrderId);
	if (index >= 0)
		FSplitter->setCollapsible(index,ACollapsible);
}

bool SplitterWidget::isHandleStretchable(int AOrderId) const
{
	return !FHandleSizes.contains(AOrderId);
}

void SplitterWidget::setHandleStretchable(int AOrderId, bool AStreatchable)
{
	if (FHandleWidgets.contains(AOrderId) && isHandleStretchable(AOrderId)!=AStreatchable)
	{
		if (!AStreatchable)
			FHandleSizes.insert(AOrderId,-1);
		else
			FHandleSizes.remove(AOrderId);
	}
}

int SplitterWidget::spacing() const
{
	return layout()->spacing();
}

void SplitterWidget::setSpacing(int ASpace)
{
	foreach(BoxWidget *boxWidget, FHandleWidgets.values())
		if (boxWidget)
			boxWidget->setSpacing(ASpace);
	layout()->setSpacing(ASpace);
}

Qt::Orientation SplitterWidget::orientation() const
{
	return FSplitter->orientation();
}

void SplitterWidget::setOrientation(Qt::Orientation AOrientation)
{
	FSplitter->setOrientation(AOrientation);
	foreach(BoxWidget *boxWidget, FHandleWidgets.values())
		boxWidget->setDirection(AOrientation==Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
}

void SplitterWidget::updateStretchFactor()
{
	for (int index=0; index<FSplitter->count(); index++)
	{
		int stretch = 0;
		BoxWidget *boxWidget = qobject_cast<BoxWidget *>(FSplitter->widget(index));
		if (boxWidget)
		{
			foreach(QWidget *widget, boxWidget->widgets())
				stretch += boxWidget->stretch(widget);
		}
		FSplitter->setStretchFactor(index,stretch);
	}
	FSplitter->refresh();
}

void SplitterWidget::saveHandleFixedSizes()
{
	QList<int> splitterSizes = FSplitter->sizes();
	for(QMap<int,int>::iterator it=FHandleSizes.begin(); it!=FHandleSizes.end(); ++it)
	{
		int index = handleIndexByOrder(it.key());
		if (index >= 0)
			it.value() = splitterSizes.value(index);
	}
}

void SplitterWidget::adjustBoxWidgetSizes(int ADeltaSize)
{
	int totalStretch = 0;
	int totalDeltaSize = ADeltaSize;

	QMap<int,int> indexStretch;
	QList<int> newSizes = FSplitter->sizes();
	for (int index=0; index<FSplitter->count(); index++)
	{
		if (newSizes.at(index) > 0)
		{
			BoxWidget *boxWidget = qobject_cast<BoxWidget *>(FSplitter->widget(index));
			int handleOrder = FHandleWidgets.key(boxWidget);
			int fixedSize = FHandleSizes.value(handleOrder);
			if (fixedSize > 0)
			{
				int &newSize = newSizes[index];
				totalDeltaSize += newSize-fixedSize;
				newSize = fixedSize;
			}
			else if (fixedSize == 0)
			{
				int stretch = (orientation()==Qt::Vertical ? boxWidget->sizePolicy().verticalStretch() : boxWidget->sizePolicy().horizontalStretch())+1;
				totalStretch += stretch;
				indexStretch.insert(index,stretch);
			}
			else
			{
				FHandleSizes[handleOrder] = newSizes.at(index);
			}
		}
	}

	for (QMap<int,int>::const_iterator it=indexStretch.constBegin(); it!=indexStretch.constEnd(); ++it)
	{
		int deltaSize = qRound(totalDeltaSize * ((double)it.value()/totalStretch));
		int &newSize = newSizes[it.key()];
		newSize = qMax(newSize+deltaSize,1);
	}

	FSplitter->setSizes(newSizes);
}

BoxWidget *SplitterWidget::getHandleBox(int AOrderId)
{
	if (FHandleWidgets.contains(AOrderId))
	{
		BoxWidget *boxWidget = FHandleWidgets[AOrderId];
		if (boxWidget == NULL)
		{
			int insertIndex = 0;
			for (QMap<int,BoxWidget *>::const_iterator it = FHandleWidgets.constBegin(); it!=FHandleWidgets.constEnd() && it.key()<AOrderId; ++it)
				if (it.value() != NULL)
					insertIndex = FSplitter->indexOf(it.value())+1;

			boxWidget = qobject_cast<BoxWidget *>(FSplitter->widget(insertIndex));
			if (boxWidget==NULL || FHandleWidgets.values().contains(boxWidget))
			{
				boxWidget = new BoxWidget(FSplitter,orientation()==Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
				boxWidget->layout()->setSpacing(layout()->spacing());
				connect(boxWidget,SIGNAL(widgetInserted(int,QWidget *)),SLOT(onBoxWidgetInserted(int,QWidget *)));
				connect(boxWidget,SIGNAL(widgetRemoved(QWidget *)),SLOT(onBoxWidgetRemoved(QWidget *)));
				FSplitter->insertWidget(insertIndex,boxWidget);
			}

			FHandleWidgets[AOrderId] = boxWidget;
		}
		return boxWidget;
	}
	return NULL;
}

int SplitterWidget::findWidgetHandle(int AOrderId) const
{
	int order = 0;
	for (QMap<int,BoxWidget *>::const_iterator it = FHandleWidgets.constBegin(); it!=FHandleWidgets.constEnd() && it.key()<=AOrderId; ++it)
		order = it.key();
	return order;
}

int SplitterWidget::handleOrderByIndex(int AIndex) const
{
	BoxWidget *boxWidget = qobject_cast<BoxWidget *>(FSplitter->widget(AIndex));
	return boxWidget!=NULL ? FHandleWidgets.key(boxWidget) : -1;
}

int SplitterWidget::handleIndexByOrder(int AOrderId) const
{
	BoxWidget *boxWidget = FHandleWidgets.value(AOrderId);
	return boxWidget!=NULL ? FSplitter->indexOf(boxWidget) : -1;
}

bool SplitterWidget::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject == FSplitter)
	{
		if (AEvent->type()==QEvent::Resize && !FHandleSizes.isEmpty())
		{
			QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(AEvent);

			int deltaSize;
			if (orientation() == Qt::Vertical)
				deltaSize = resizeEvent->size().height() - resizeEvent->oldSize().height();
			else
				deltaSize = resizeEvent->size().width() - resizeEvent->oldSize().width();

			if (deltaSize != 0)
				adjustBoxWidgetSizes(deltaSize);
		}
	}
	return QFrame::eventFilter(AObject,AEvent);
}

void SplitterWidget::onSplitterMoved(int APosition, int AIndex)
{
	Q_UNUSED(APosition);
	saveHandleFixedSizes();
	emit handleMoved(handleOrderByIndex(AIndex),FSplitter->sizes().value(AIndex));
}

void SplitterWidget::onBoxWidgetInserted(int AOrderId, QWidget *AWidget)
{
	BoxWidget *boxWidget = qobject_cast<BoxWidget *>(sender());
	if (boxWidget)
	{
		boxWidget->setVisible(true);
		if (!FMovingWidgets)
		{
			insertHandle(FWidgetHandles.value(AWidget));
			FWidgetOrders.insert(AOrderId,AWidget);
			emit widgetInserted(AWidget);
		}
	}
}

void SplitterWidget::onBoxWidgetRemoved(QWidget *AWidget)
{
	BoxWidget *boxWidget = qobject_cast<BoxWidget *>(sender());
	if (boxWidget)
	{
		boxWidget->setVisible(!boxWidget->isEmpty());
		if (!FMovingWidgets)
		{
			removeHandle(FWidgetHandles.value(AWidget));
			FWidgetOrders.remove(FWidgetOrders.key(AWidget));
			emit widgetRemoved(AWidget);
		}
	}
}
