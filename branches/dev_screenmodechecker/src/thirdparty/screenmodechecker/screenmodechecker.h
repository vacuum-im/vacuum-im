#ifndef SCREENMODECHECKER_H
#define SCREENMODECHECKER_H

#include <QObject>

// Linux
#if defined(Q_WS_X11)

class ScreenModeChecker : public QObject
{
	Q_OBJECT
public:
	ScreenModeChecker() {}
	~ScreenModeChecker() {}

	bool isFullscreenAppActive();
	bool isScreensaverActive();
	bool isDummy() { return false; }
};

// Windows
#elif defined(Q_WS_WIN)

#include <windows.h>

class ScreenModeChecker : public QObject
{
	Q_OBJECT
public:
	ScreenModeChecker() {}
	~ScreenModeChecker() {}

	bool isFullscreenAppActive();
	bool isScreensaverActive();
	bool isDummy() { return false; }
};

#else
// Dummy

class ScreenModeChecker : public QObject
{
	Q_OBJECT
public:
	ScreenModeChecker() {}
	~ScreenModeChecker() {}

	bool isFullscreenAppActive() { return false; }
	bool isScreensaverActive() { return false; }
	bool isDummy() { return true; }
};
#endif

#endif // SCREENMODECHECKER_H
