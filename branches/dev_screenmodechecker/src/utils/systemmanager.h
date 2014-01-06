#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include <QObject>
#include "utilsexport.h"

class UTILS_EXPORT SystemManager : 
	public QObject
{
	Q_OBJECT;
	struct SystemManagerData;
public:
	static SystemManager *instance();
	//Idle
	static int systemIdle();
	static bool isSystemIdleActive();
	//ScreenModeChecker
	static bool isScreenSaverRunning();
	static bool isFullscreenAppActive();
public:
	static void startSystemIdle();
	static void stopSystemIdle();
signals:
	void systemIdleChanged(int ASeconds);
protected slots:
	void onIdleChanged(int ASeconds);
private:
	static SystemManagerData *d;
};

#endif // SYSTEMMANAGER_H