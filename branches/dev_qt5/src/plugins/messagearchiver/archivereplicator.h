#ifndef ARCHIVEREPLICATOR_H
#define ARCHIVEREPLICATOR_H

#include <QTimer>
#include <interfaces/imessagearchiver.h>
#include "replicateworker.h"

class ArchiveReplicator :
	public QObject
{
	Q_OBJECT;
public:
	ArchiveReplicator(IMessageArchiver *AArchiver, const Jid &AStreamJid, QObject *AParent);
	~ArchiveReplicator();
	void quitAndDestroy();
protected:
	QString replicationDatabasePath() const;
	QString replicationDatabaseConnection() const;
	void connectEngine(IArchiveEngine *AEngine);
	void disconnectEngine(IArchiveEngine *AEngine);
protected:
	void startSyncModifications();
	void startSyncCollections();
	void startNextModification();
	void stopReplication(const QUuid &AEngineId);
protected slots:
	void onStartReplicateTimerTimeout();
protected slots:
	void onReplicateWorkerReady();
	void onReplicateWorkerFinished();
	void onReplicateWorkerTaskFinished(ReplicateTask *ATask);
protected slots:
	void onEngineRequestFailed(const QString &AId, const XmppError &AError);
	void onEngineCollectionSaved(const QString &AId, const IArchiveCollection &ACollection);
	void onEngineCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void onEngineCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void onEngineModificationsLoaded(const QString &AId, const IArchiveModifications &AModifications);
private:
	IMessageArchiver *FArchiver;
private:
	bool FDestroy;
	Jid FStreamJid;
	ReplicateWorker *FWorker;
	QTimer FStartReplicateTimer;
	QMap<QUuid, IArchiveEngine *> FEngines;
	QList<IArchiveEngine *> FConnectedEngines;
private:
	QMap<QString, QUuid> FLoadStateTasks;
	QMap<QString, QUuid> FSaveModifsTasks;
	QMap<QString, QUuid> FLoadModifsRequests;
	QMap<QString, QList<QUuid> > FLoadModifsTasks;
	QMap<QString, QUuid> FSaveCollectionRequests;
	QMap<QString, QUuid> FLoadCollectionRequests;
	QMap<QString, QUuid> FRemoveCollectionRequests;
	QMap<QString, QUuid> FUpdateVersionTasks;
private:
	QList<QUuid> FSourceOrder;
	QList<QUuid> FDestinations;
	ReplicateModification FModification;
	QList<ReplicateModification> FModifications;
};

#endif // ARCHIVEREPLICATOR_H
