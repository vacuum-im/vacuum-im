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
	static int systemIdle();
	static bool isSystemIdleActive();
	static void startSystemIdle();
	static void stopSystemIdle();
signals:
	void systemIdleChanged(int ASeconds);
protected slots:
	void onIdleChanged(int ASeconds);
private:
	SystemManager();
	~SystemManager();
	SystemManagerData *d;
};

#endif // SYSTEMMANAGER_H
