#include <windows.h>
#include "screenmodechecker.h"

bool ScreenModeChecker::isFullscreenAppActive()
{
	HWND hWnd = GetForegroundWindow();
	if (NULL == hWnd)
		return false;

	int cx = GetSystemMetrics(SM_CXSCREEN);
	int cy = GetSystemMetrics(SM_CYSCREEN);
	RECT r;
	GetWindowRect(hWnd, &r);

	return (r.right - r.left == cx && r.bottom - r.top == cy);
}

bool ScreenModeChecker::isScreensaverActive()
{
	BOOL ret;
	SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &ret, 0);

	return (0 != ret);
}
