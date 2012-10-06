#include "toolbarchanger.h"

#include <QTimer>
#include <QPointer>
#include <QWidgetAction>

class ToolButtonAction :
	public QWidgetAction 
{
public:
	ToolButtonAction(Action *AAction) : QWidgetAction(AAction)
	{
		FAction = AAction;
		FDefaultButton = NULL;
	}
	QToolButton *defaultToolButton()
	{
		return FDefaultButton;
	}
	QWidget *createWidget(QWidget *AParent)
	{
		QToolButton *button = new QToolButton(AParent);
		if (FDefaultButton.isNull())
		{
			QToolBar *toolbar = qobject_cast<QToolBar *>(AParent);
			if (toolbar)
				button->setToolButtonStyle(toolbar->toolButtonStyle());
			button->setDefaultAction(FAction);
			FDefaultButton = button;
		}
		else
		{
			button->setToolButtonStyle(FDefaultButton->toolButtonStyle());
			button->setDefaultAction(FDefaultButton->defaultAction());
			button->setPopupMode(FDefaultButton->popupMode());
			button->setAutoRaise(FDefaultButton->autoRaise());
			button->setArrowType(FDefaultButton->arrowType());
		}
		return button;
	}
private:
	Action *FAction;
	QPointer<QToolButton> FDefaultButton;
};


ToolBarChanger::ToolBarChanger(QToolBar *AToolBar) : QObject(AToolBar)
{
	FSeparatorsVisible = true;
	FAutoHideIfEmpty = true;

	FToolBar = AToolBar;
	FToolBar->clear();

	QWidget *widget = new QWidget(FToolBar);
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	FAllignChange = insertWidget(widget, TBG_ALLIGN_CHANGE);
	FAllignChange->setVisible(false);

	updateVisible();
}

ToolBarChanger::~ToolBarChanger()
{
	emit toolBarChangerDestroyed(this);
}

bool ToolBarChanger::isEmpty() const
{
	return FWidgets.count() < 2;
}

bool ToolBarChanger::separatorsVisible() const
{
	return FSeparatorsVisible;
}

void ToolBarChanger::setSeparatorsVisible(bool ASeparatorsVisible)
{
	FSeparatorsVisible = ASeparatorsVisible;
	foreach(QAction *separator, FSeparators.values())
		separator->setVisible(ASeparatorsVisible);
	updateSeparatorVisible();
}

bool ToolBarChanger::autoHideEmptyToolbar() const
{
	return FAutoHideIfEmpty;
}

void ToolBarChanger::setAutoHideEmptyToolbar(bool AAutoHide)
{
	FAutoHideIfEmpty = AAutoHide;
	updateVisible();
}

QToolBar *ToolBarChanger::toolBar() const
{
	return FToolBar;
}

int ToolBarChanger::itemGroup(QAction *AHandle) const
{
	return FWidgets.key(FHandles.key(AHandle));
}

QList<QAction *> ToolBarChanger::groupItems(int AGroup) const
{
	QList<QAction *> handles;
	QList<QWidget *> widgets = AGroup!=TBG_NULL ? FWidgets.values(AGroup) : FWidgets.values();
	foreach(QWidget *widget, widgets)
		handles.append(FHandles.value(widget));
	return handles;
}

QAction *ToolBarChanger::widgetHandle(QWidget *AWidget) const
{
	return FHandles.value(AWidget);
}

QWidget *ToolBarChanger::handleWidget(QAction *AHandle) const
{
	return FHandles.key(AHandle);
}

QAction *ToolBarChanger::actionHandle(Action *AAction) const
{
	return FHandles.value(FButtons.value(AAction));
}

Action *ToolBarChanger::handleAction(QAction *AHandle) const
{
	return FButtons.key(qobject_cast<QToolButton *>(FHandles.key(AHandle)));
}

QAction *ToolBarChanger::insertWidget(QWidget *AWidget, int AGroup)
{
	if (!FHandles.contains(AWidget))
	{
		QAction *before = findGroupSeparator(AGroup);
		QAction *handle = before!=NULL ? FToolBar->insertWidget(before, AWidget) : FToolBar->addWidget(AWidget);
		insertGroupSeparator(AGroup,handle);

		FWidgets.insertMulti(AGroup,AWidget);
		FHandles.insert(AWidget, handle);
		connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));

		emit itemInserted(before,handle,NULL,AWidget,AGroup);
		updateVisible();
	}
	return FHandles.value(AWidget);
}

QToolButton *ToolBarChanger::insertAction(Action *AAction, int AGroup)
{
	if (!FButtons.contains(AAction))
	{
		ToolButtonAction *buttonAction = new ToolButtonAction(AAction);

		QAction *before = findGroupSeparator(AGroup);
		if (before)
			FToolBar->insertAction(before, buttonAction);
		else
			FToolBar->addAction(buttonAction);
		insertGroupSeparator(AGroup,buttonAction);

		QToolButton *button = buttonAction->defaultToolButton();
		FWidgets.insert(AGroup,button);
		FHandles.insert(button,buttonAction);
		FButtons.insert(AAction,button);
		connect(button,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));

		emit itemInserted(before,buttonAction,AAction,button,AGroup);
		updateVisible();
	}
	return FButtons.value(AAction);
}

void ToolBarChanger::removeItem(QAction *AHandle)
{
	QWidget *widget = FHandles.key(AHandle, NULL);
	if (widget && AHandle!=FAllignChange)
	{
		disconnect(widget,SIGNAL(destroyed(QObject *)),this,SLOT(onWidgetDestroyed(QObject *)));
		FToolBar->removeAction(AHandle);

		QMap<Action *, QToolButton *>::iterator it = FButtons.begin();
		while (it != FButtons.end())
		{
			if (it.value() == widget)
			{
				it.key()->deleteLater();
				it = FButtons.erase(it);
			}
			else
			{
				it++;
			}
		}
		FHandles.take(widget)->deleteLater();

		int group = FWidgets.key(widget);
		FWidgets.remove(group,widget);
		removeGroupSeparator(group);

		emit itemRemoved(AHandle);
		updateVisible();
	}
}

void ToolBarChanger::clear()
{
	foreach(QAction *handle, FHandles.values())
		removeItem(handle);
	FToolBar->clear();
}

void ToolBarChanger::updateVisible()
{
	FToolBar->setMaximumWidth(!FAutoHideIfEmpty || !isEmpty() ? QWIDGETSIZE_MAX : 0);
	FToolBar->setMaximumHeight(!FAutoHideIfEmpty || !isEmpty() ? QWIDGETSIZE_MAX : 0);
}

void ToolBarChanger::updateSeparatorVisible()
{
	const QList<QAction *> separators = FSeparators.values();
	if (FSeparators.count() > 2)
		separators.at(1)->setVisible(FSeparatorsVisible);
	separators.first()->setVisible(false);
	FSeparators.value(TBG_ALLIGN_CHANGE)->setVisible(false);
}

QAction *ToolBarChanger::findGroupSeparator(int AGroup) const
{
	QMap<int, QAction *>::const_iterator it = FSeparators.upperBound(AGroup);
	return it!=FSeparators.end() ? it.value() : NULL;
}

void ToolBarChanger::insertGroupSeparator(int AGroup, QAction *ABefore)
{
	if (AGroup > TBG_ALLIGN_CHANGE)
	{
		FAllignChange->setVisible(true);
	}
	if (!FSeparators.contains(AGroup))
	{
		QAction *separator = FToolBar->insertSeparator(ABefore);
		separator->setVisible(FSeparatorsVisible);
		FSeparators.insert(AGroup, separator);
		updateSeparatorVisible();
	}
}

void ToolBarChanger::removeGroupSeparator(int AGroup)
{
	if (!FWidgets.contains(AGroup))
	{
		QAction *separator = FSeparators.take(AGroup);
		FToolBar->removeAction(separator);
		delete separator;
		updateSeparatorVisible();
	}
	if (FWidgets.keys().last()<=TBG_ALLIGN_CHANGE)
	{
		FAllignChange->setVisible(false);
	}
}

void ToolBarChanger::onWidgetDestroyed(QObject *AObject)
{
	foreach(QWidget *widget, FWidgets.values())
		if (qobject_cast<QObject *>(widget) == AObject)
			removeItem(FHandles.value(widget));
}
