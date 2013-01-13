#include "screenmodechecker.h"

bool ScreenModeChecker::isFullscreenAppActive()
{
	return IsFullScreenMode();
}

bool ScreenModeChecker::isScreensaverActive()
{
	return CheckStateIsLocked();
}
