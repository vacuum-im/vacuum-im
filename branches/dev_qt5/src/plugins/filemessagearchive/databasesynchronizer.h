#ifndef DATABASESYNCHRONIZER_H
#define DATABASESYNCHRONIZER_H

#include <QQueue>
#include <QMutex>
#include <QThread>
#include <interfaces/ifilemessagearchive.h>
#include "databaseworker.h"

class DatabaseSynchronizer :
	public QThread
{
	Q_OBJECT;
public:
	DatabaseSynchronizer(IFileMessageArchive *AFileArchive, DatabaseWorker *ADatabaseWorker, QObject *AParent);
	~DatabaseSynchronizer();
	void startSync(const Jid &AStreamJid);
	void removeSync(const Jid &AStreamJid);
public slots:
	void quit();
signals:
	void syncFinished(const Jid &AStreamJid, bool AFailed);
protected:
	void run();
private:
	bool FQuit;
	QMutex FMutex;
	QQueue<Jid> FStreams;
	DatabaseWorker *FDatabaseWorker;
	IFileMessageArchive *FFileArchive;
};

#endif // DATABASESYNCHRONIZER_H
