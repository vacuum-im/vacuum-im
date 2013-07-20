#include "x11info.h"

#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <X11/Xlib.h>


X11Info::X11Info()
{
}

/*!
	Returns the horizontal resolution of the given \a screen in terms of the
	number of dots per inch.

	The \a screen argument is an X screen number. Be aware that if
	the user's system uses Xinerama (as opposed to traditional X11
	multiscreen), there is only one X screen. Use QDesktopWidget to
	query for information about Xinerama screens.

	\sa setAppDpiX(), appDpiY()
*/
int X11Info::appDpiX(int AScreen)
{
	if (AScreen == -1)
	{
		const QScreen *scr = QGuiApplication::primaryScreen();
		return scr!=NULL ? qRound(scr->logicalDotsPerInchX()) : 75;
	}

	QList<QScreen *> screens = QGuiApplication::screens();
	return AScreen < screens.size() ? screens[AScreen]->logicalDotsPerInchX() : 0;
}

/*!
	Returns the vertical resolution of the given \a screen in terms of the
	number of dots per inch.

	The \a screen argument is an X screen number. Be aware that if
	the user's system uses Xinerama (as opposed to traditional X11
	multiscreen), there is only one X screen. Use QDesktopWidget to
	query for information about Xinerama screens.

	\sa setAppDpiY(), appDpiX()
*/
int X11Info::appDpiY(int AScreen)
{
	if (AScreen == -1)
	{
		const QScreen *scr = QGuiApplication::primaryScreen();
		return scr !=NULL ? qRound(scr->logicalDotsPerInchY()) : 75;
	}

	QList<QScreen *> screens = QGuiApplication::screens();
	return AScreen < screens.size() ? screens[AScreen]->logicalDotsPerInchY() : 0;
}

/*!
	Returns a handle for the applications root window on the given \a screen.

	The \a screen argument is an X screen number. Be aware that if
	the user's system uses Xinerama (as opposed to traditional X11
	multiscreen), there is only one X screen. Use QDesktopWidget to
	query for information about Xinerama screens.

	\sa QApplication::desktop()
*/
unsigned long X11Info::appRootWindow(int AScreen)
{
#if 0
	// This looks like it should work, but gives the wrong value.
	QDesktopWidget *desktop = QApplication::desktop();
	QWidget *screenWidget = desktop->screen(screen);
	QWindow *window = screenWidget->windowHandle();
#else
	Q_UNUSED(AScreen);
	QDesktopWidget *desktop = QApplication::desktop();
	QWindow *window = desktop->windowHandle();
#endif
	return window->winId();
}

/*!
	Returns the number of the screen where the application is being
	displayed.

	\sa display(), screen()
*/
int X11Info::appScreen()
{
	QDesktopWidget *desktop = QApplication::desktop();
	return desktop->primaryScreen();
}

/*!
	Returns the default display for the application.

	\sa appScreen()
*/
Display *X11Info::display()
{
	static Display *xdisplay = XOpenDisplay(NULL);
	return xdisplay;
}
