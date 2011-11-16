#include "widgetmanager.h"

#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>

#ifdef Q_WS_X11
	#include <QX11Info>
	#include <X11/Xutil.h>
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>

	#define MESSAGE_SOURCE_OLD            0
	#define MESSAGE_SOURCE_APPLICATION    1
	#define MESSAGE_SOURCE_PAGER          2
#endif //Q_WS_X11

namespace WidgetManagerData 
{
	static bool isAlertEnabled = true;
};

class WindowSticker : 
	public QObject
{
public:
	WindowSticker();
	static WindowSticker *instance();
	void insertWindow(QWidget *AWindow);
	void removeWindow(QWidget *AWindow);
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	int FStickEvent;
	QPoint FStickPos;
	QWidget *FCurWindow;
};

WindowSticker::WindowSticker()
{
	FCurWindow = NULL;
	FStickEvent = QEvent::registerEventType();
}

WindowSticker *WindowSticker::instance()
{
	static WindowSticker *sticker = NULL;
	if (!sticker)
		sticker = new WindowSticker;
	return sticker;
}

void WindowSticker::insertWindow(QWidget *AWindow)
{
	if (AWindow)
	{
		AWindow->installEventFilter(this);
	}
}

void WindowSticker::removeWindow(QWidget *AWindow)
{
	if (AWindow)
	{
		AWindow->removeEventFilter(this);
	}
}

bool WindowSticker::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::NonClientAreaMouseButtonPress)
	{
		QWidget *window = qobject_cast<QWidget *>(AWatched);
		if (window && window->isWindow())
		{
			FCurWindow = window;
		}
	}
	else if (AEvent->type() == QEvent::NonClientAreaMouseButtonRelease)
	{
		FCurWindow = NULL;
	}
	else if (AEvent->type() == QEvent::NonClientAreaMouseMove)
	{
		FCurWindow = NULL;
	}
	else if (AEvent->type() == QEvent::WindowStateChange)
	{
		FCurWindow = NULL;
	}
	else if (AWatched==FCurWindow && AEvent->type()==QEvent::Move)
	{
		const int delta = 15;
		QPoint cursorPos = QCursor::pos();
		QRect windowRect = FCurWindow->frameGeometry();
		QRect desckRect = QApplication::desktop()->availableGeometry(FCurWindow);

		int borderTop = cursorPos.y() - windowRect.y();
		int borderLeft = cursorPos.x() - windowRect.x();
		int borderRight = cursorPos.x() + desckRect.right() - windowRect.right();
		int borderBottom = cursorPos.y() + desckRect.bottom() - windowRect.bottom();

		FStickPos = windowRect.topLeft();
		if (qAbs(borderTop - cursorPos.y()) < delta)
		{
			FStickPos.setY(0);
		}
		else if (qAbs(borderBottom - cursorPos.y()) < delta)
		{
			FStickPos.setY(desckRect.bottom() - windowRect.height());
		}
		if (qAbs(borderLeft - cursorPos.x()) < delta)
		{
			FStickPos.setX(0);
		}
		else if(qAbs(borderRight - cursorPos.x()) < delta)
		{
			FStickPos.setX(desckRect.right() - windowRect.width());
		}

		if (FStickPos != windowRect.topLeft())
		{
			QEvent *stickEvent = new QEvent((QEvent::Type)FStickEvent);
			QApplication::postEvent(AWatched,stickEvent,Qt::HighEventPriority);
		}
	}
	else if (FCurWindow==AWatched && AEvent->type()==FStickEvent)
	{
		FCurWindow->move(FStickPos);
		return true;
	}
	return QObject::eventFilter(AWatched,AEvent);
}

void WidgetManager::raiseWidget(QWidget *AWidget)
{
#ifdef Q_WS_X11
	static Atom         NET_ACTIVE_WINDOW = 0;
	XClientMessageEvent xev;

	if (NET_ACTIVE_WINDOW == 0)
	{
		Display *dpy      = QX11Info::display();
		NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	}

	xev.type         = ClientMessage;
	xev.window       = AWidget->winId();
	xev.message_type = NET_ACTIVE_WINDOW;
	xev.format       = 32;
	xev.data.l[0]    = MESSAGE_SOURCE_PAGER;
	xev.data.l[1]    = QX11Info::appUserTime();
	xev.data.l[2]    = xev.data.l[3] = xev.data.l[4] = 0;

	XSendEvent(QX11Info::display(), QX11Info::appRootWindow(), False, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent*)&xev);
#endif //Q_WS_X11

	AWidget->raise();
}

void WidgetManager::showActivateRaiseWindow(QWidget *AWindow)
{
	if (AWindow->isVisible())
	{
		if (AWindow->isMinimized())
		{
			if (AWindow->isMaximized())
				AWindow->showMaximized();
			else
				AWindow->showNormal();
		}
	}
	else
	{
		AWindow->show();
	}
	AWindow->activateWindow();
	WidgetManager::raiseWidget(AWindow);
}

void WidgetManager::setWindowSticky( QWidget *AWindow, bool ASticky )
{
#ifdef Q_WS_WIN
	if (ASticky)
		WindowSticker::instance()->insertWindow(AWindow);
	else
		WindowSticker::instance()->removeWindow(AWindow);
#else
	Q_UNUSED(AWindow);
	Q_UNUSED(ASticky);
#endif
}

void WidgetManager::alertWidget(QWidget *AWidget)
{
	if (AWidget!=NULL && isWidgetAlertEnabled())
		QApplication::alert(AWidget);
}

bool WidgetManager::isWidgetAlertEnabled()
{
	return WidgetManagerData::isAlertEnabled;
}

void WidgetManager::setWidgetAlertEnabled(bool AEnabled)
{
	WidgetManagerData::isAlertEnabled = AEnabled;
}

Qt::Alignment WidgetManager::windowAlignment(const QWidget *AWindow)
{
	static const int delta = 4;
	Qt::Alignment align = 0;
	QRect windowRect = AWindow->frameGeometry();
	QRect screenRect = QApplication::desktop()->availableGeometry(AWindow);
	if (!screenRect.isEmpty() && !windowRect.isEmpty())
	{
		if (qAbs(screenRect.left() - windowRect.left()) < delta)
			align |= Qt::AlignLeft;
		else if (qAbs(screenRect.right() - windowRect.right()) < delta)
			align |= Qt::AlignRight;
		if (qAbs(screenRect.top() - windowRect.top()) < delta)
			align |= Qt::AlignTop;
		else if (qAbs(screenRect.bottom() - windowRect.bottom()) < delta)
			align |= Qt::AlignBottom;
	}
	return align;
}

void WidgetManager::alignWindow(QWidget *AWindow, Qt::Alignment AAlign)
{
#ifndef Q_WS_X11
	if (AAlign > 0)
	{
		QRect frameRect = AWindow->frameGeometry();
		QRect windowRect = AWindow->geometry();
		QRect rect = alignGeometry(frameRect.size(),AWindow,AAlign);
		rect.adjust(windowRect.left()-frameRect.left(),windowRect.top()-frameRect.top(),windowRect.right()-frameRect.right(),windowRect.bottom()-frameRect.bottom());
		AWindow->setGeometry(rect);
	}
#else
	Q_UNUSED(AWindow);
	Q_UNUSED(AAlign);
#endif
}

QRect WidgetManager::alignGeometry(const QSize &ASize, const QWidget *AWidget, Qt::Alignment AAlign)
{
	QRect availRect = AWidget!=NULL ? QApplication::desktop()->availableGeometry(AWidget) : QApplication::desktop()->availableGeometry();
	Qt::LayoutDirection direction = AWidget!=NULL ? AWidget->layoutDirection() : QApplication::layoutDirection();
	return QStyle::alignedRect(direction,AAlign,ASize.boundedTo(availRect.size()),availRect);
}
