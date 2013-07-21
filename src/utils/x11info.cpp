#include "x11info.h"

#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <X11/Xlib.h>

#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

X11Info::X11Info()
{
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
	Returns the default display for the application.

	\sa appScreen()
*/
Display *X11Info::display()
{
	QPlatformNativeInterface *native = QApplication::platformNativeInterface();
	void *display = native!=NULL ? native->nativeResourceForScreen(QByteArray("display"), QGuiApplication::primaryScreen()) : NULL;
	return reinterpret_cast<Display *>(display);
}
