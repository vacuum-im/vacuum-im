#ifndef SCREENSTATE_H
#define SCREENSTATE_H

#include <QObject>

// Linux
#if defined(Q_WS_X11)
class ScreenState : public QObject
{
	Q_OBJECT
public:
	ScreenState() {}
	~ScreenState() {}

	bool isFullscreenAppActive();
	bool isScreenSaverRunning();
	bool isDummy() { return false; }
};

// Windows
#elif defined(Q_WS_WIN)
#include <windows.h>
class ScreenState : public QObject
{
	Q_OBJECT
public:
	ScreenState() {}
	~ScreenState() {}

	bool isFullscreenAppActive();
	bool isScreenSaverRunning();
	bool isDummy() { return false; }
};

// MacOSX
/* #elif defined(Q_WS_MACX)
#include "mactools.h"
class ScreenState : public QObject
{
	Q_OBJECT
public:
	ScreenState() { InitTools(); }
	~ScreenState() { StopTools(); }

	bool isFullscreenAppActive();
	bool isScreenSaverRunning();
	bool isDummy() { return false; }
}; */

#else
// Dummy
class ScreenState : public QObject
{
	Q_OBJECT
public:
	ScreenState() {}
	~ScreenState() {}

	bool isFullscreenAppActive() { return false; }
	bool isScreenSaverRunning() { return false; }
	bool isDummy() { return true; }
};
#endif

#endif // SCREENSTATE_H
