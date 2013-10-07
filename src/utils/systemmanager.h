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
public:
	void startSystemIdle();
	void stopSystemIdle();
signals:
	void systemIdleChanged(int ASeconds);
protected slots:
	void onIdleChanged(int ASeconds);
private:
	static SystemManagerData *d;
};

#endif // SYSTEMMANAGER_H
