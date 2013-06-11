#include <QX11Info>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include "x11tools.h"
#include "screenstate.h"

bool ScreenState::isFullscreenAppActive()
{
	return X11_checkFullScreen(QX11Info::display());
}

bool ScreenState::isScreenSaverRunning()
{
	// org.freedesktop.ScreenSaver
	{
		QDBusInterface dbus("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
		if (dbus.isValid())
		{
			QDBusReply<bool> reply = dbus.call("GetActive");
			if (reply.isValid() && reply.value())
				return true;
		}
	}
	// org.kde.screensaver
	{
		QDBusInterface dbus("org.kde.screensaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
		if (dbus.isValid())
		{
			QDBusReply<bool> reply = dbus.call("GetActive");
			if (reply.isValid() && reply.value())
				return true;
		}
	}
	// org.gnome.ScreenSaver
	{
		QDBusInterface dbus("org.gnome.ScreenSaver", "/", "org.gnome.ScreenSaver", QDBusConnection::sessionBus());
		if (dbus.isValid())
		{
			QDBusReply<bool> reply = dbus.call("GetActive");
			if (reply.isValid() && reply.value())
				return true;
		}
	}

	return false;
}
