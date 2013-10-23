#include "archivereplicator.h"

#include <definitions/optionvalues.h>
#include <utils/options.h>
#include <utils/datetime.h>

#define DATABASE_FILE_NAME              "replication.db"

#define REPLICATE_START_TIMEOUT         10*1000
#define REPLICATE_RESTART_TIMEOUT       5*60*1000
#define REPLICATE_MODIFICATIONS_COUNT   50

ArchiveReplicator::ArchiveReplicator(IMessageArchiver *AArchiver, const Jid &AStreamJid, QObject *AParent) : QObject(AParent)
{
	FWorker = NULL;
	FDestroy = false;

	FArchiver = AArchiver;
	FStreamJid = AStreamJid;

	FStartReplicateTimer.setSingleShot(true);
	connect(&FStartReplicateTimer,SIGNAL(timeout()),SLOT(onStartReplicateTimerTimeout()));

	FStartReplicateTimer.start(REPLICATE_START_TIMEOUT);
}

ArchiveReplicator::~ArchiveReplicator()
{
	delete FWorker;
}

void ArchiveReplicator::quitAndDestroy()
{
	FDestroy = true;
	if (FWorker && !FEngines.isEmpty())
	{
		foreach(const QUuid &engineId, FEngines.keys())
			stopReplication(engineId);
	}
	else
	{
		deleteLater();
	}
}

QString ArchiveReplicator::replicationDatabasePath() const
{
	QString dirPath = FArchiver->archiveDirPath(FStreamJid);
	if (!dirPath.isEmpty())
		return dirPath + "/"DATABASE_FILE_NAME;
	return QString::null;
}

QString ArchiveReplicator::replicationDatabaseConnection() const
{
	return QString("ArchiveReplicationDatabase-%1").arg(FStreamJid.pBare());
}

void ArchiveReplicator::connectEngine(IArchiveEngine *AEngine)
{
	if (AEngine && !FConnectedEngines.contains(AEngine))
	{
		connect(AEngine->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onEngineRequestFailed(const QString &, const XmppError &)));
		connect(AEngine->instance(),SIGNAL(collectionSaved(const QString &, const IArchiveCollection &)),SLOT(onEngineCollectionSaved(const QString &, const IArchiveCollection &)));
		connect(AEngine->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),SLOT(onEngineCollectionLoaded(const QString &, const IArchiveCollection &)));
		connect(AEngine->instance(),SIGNAL(collectionsRemoved(const QString &, const IArchiveRequest &)),SLOT(onEngineCollectionsRemoved(const QString &, const IArchiveRequest &)));
		connect(AEngine->instance(),SIGNAL(modificationsLoaded(const QString &, const IArchiveModifications &)),SLOT(onEngineModificationsLoaded(const QString &, const IArchiveModifications &)));
		FConnectedEngines.append(AEngine);
	}
}

void ArchiveReplicator::disconnectEngine( IArchiveEngine *AEngine )
{
	if (FConnectedEngines.contains(AEngine))
	{
		disconnect(AEngine->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),this,SLOT(onEngineRequestFailed(const QString &, const XmppError &)));
		disconnect(AEngine->instance(),SIGNAL(collectionSaved(const QString &, const IArchiveCollection &)),this,SLOT(onEngineCollectionSaved(const QString &, const IArchiveCollection &)));
		disconnect(AEngine->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),this,SLOT(onEngineCollectionLoaded(const QString &, const IArchiveCollection &)));
		disconnect(AEngine->instance(),SIGNAL(collectionsRemoved(const QString &, const IArchiveRequest &)),this,SLOT(onEngineCollectionsRemoved(const QString &, const IArchiveRequest &)));
		disconnect(AEngine->instance(),SIGNAL(modificationsLoaded(const QString &, const IArchiveModifications &)),this,SLOT(onEngineModificationsLoaded(const QString &, const IArchiveModifications &)));
		FConnectedEngines.removeAll(AEngine);
	}
}

void ArchiveReplicator::startSyncModifications()
{
	foreach(IArchiveEngine *engine, FEngines.values())
	{
		ReplicateTaskLoadState *task = new ReplicateTaskLoadState(engine->engineId());
		if (FWorker->startTask(task))
			FLoadStateTasks.insert(task->taskId(),engine->engineId());
		else
			stopReplication(engine->engineId());
	}
}

void ArchiveReplicator::startSyncCollections()
{
	if (FLoadModifsRequests.isEmpty() && FSaveModifsTasks.isEmpty())
	{
		QList<QUuid> engines;
		foreach(const QUuid &engineId, FEngines.keys())
		{
			if (Options::node(OPV_HISTORY_ENGINE_ITEM,engineId.toString()).value("replicate-append").toBool())
				engines.append(engineId);
			else if (Options::node(OPV_HISTORY_ENGINE_ITEM,engineId.toString()).value("replicate-remove").toBool())
				engines.append(engineId);
			else
				stopReplication(engineId);
		}

		if (!engines.isEmpty())
		{
			ReplicateTaskLoadModifications *task = new ReplicateTaskLoadModifications(engines);
			if (FWorker->startTask(task))
				FLoadModifsTasks.insert(task->taskId(),engines);
			else foreach(const QUuid engineId, engines)
				stopReplication(engineId);
		}
	}
}

void ArchiveReplicator::startNextModification()
{
	while(FDestinations.isEmpty() && !FModifications.isEmpty())
	{
		FModification = FModifications.takeFirst();
		if (FModification.action == IArchiveModification::Changed)
		{
			foreach(const QUuid &engineId, FModification.destinations)
			{
				if (FEngines.contains(engineId) && Options::node(OPV_HISTORY_ENGINE_ITEM,engineId.toString()).value("replicate-append").toBool())
					FDestinations.append(engineId);
			}
			for (int i=0; !FDestinations.isEmpty() && FLoadCollectionRequests.isEmpty() && i<FSourceOrder.count(); i++)
			{
				IArchiveEngine *engine = FModification.sources.contains(FSourceOrder.at(i)) ? FArchiver->findArchiveEngine(FSourceOrder.at(i)) : NULL;
				if (engine)
				{
					QString requestId = engine->loadCollection(FStreamJid,FModification.header);
					if (!requestId.isEmpty())
						FLoadCollectionRequests.insert(requestId,engine->engineId());
				}
			}
			if (FLoadCollectionRequests.isEmpty())
				FDestinations.clear();
		}
		else if (FModification.action == IArchiveModification::Removed)
		{
			foreach(const QUuid &engineId, FModification.destinations)
			{
				IArchiveEngine *engine = FEngines.value(engineId);
				if (engine!=NULL && Options::node(OPV_HISTORY_ENGINE_ITEM,engineId.toString()).value("replicate-remove").toBool())
				{
					IArchiveRequest request;
					request.with = FModification.header.with;
					request.start = FModification.header.start;
					QString requestId = engine->removeCollections(FStreamJid,request);
					if (!requestId.isEmpty())
					{
						FDestinations.append(engineId);
						FRemoveCollectionRequests.insert(requestId,engineId);
					}
				}
			}
		}
	}

	if (FDestinations.isEmpty() && FModifications.isEmpty())
	{
		foreach(const QUuid &engineId, FEngines.keys())
			stopReplication(engineId);
	}
}

void ArchiveReplicator::stopReplication(const QUuid &AEngineId)
{
	IArchiveEngine *engine = FEngines.take(AEngineId);
	if (engine)
	{
		if (FWorker && FEngines.isEmpty())
			FWorker->quit();

		for (QList<ReplicateModification>::iterator it=FModifications.begin(); it!=FModifications.end(); )
		{
			it->destinations.removeAll(AEngineId);
			if (it->destinations.isEmpty())
				it = FModifications.erase(it);
			else
				it ++;
		}

		FDestinations.removeAll(AEngineId);
	}
}

void ArchiveReplicator::onStartReplicateTimerTimeout()
{
	FStartReplicateTimer.start(REPLICATE_RESTART_TIMEOUT);
	if (FWorker==NULL && FArchiver->isReady(FStreamJid) && Options::node(OPV_HISTORY_STREAM_ITEM,FStreamJid.pBare()).node("replicate").value().toBool())
	{
		int replCount = 0;
		int manualCount = 0;
		foreach(IArchiveEngine *engine, FArchiver->archiveEngines())
		{
			if (FArchiver->isArchiveEngineEnabled(engine->engineId()) && engine->isCapable(FStreamJid,IArchiveEngine::ArchiveManagement))
			{
				if (engine->isCapable(FStreamJid,IArchiveEngine::ArchiveReplication))
				{
					replCount++;
					connectEngine(engine);
					FEngines.insert(engine->engineId(),engine);
				}
				else if (engine->isCapable(FStreamJid,IArchiveEngine::ManualArchiving))
				{
					manualCount++;
					connectEngine(engine);
					FEngines.insert(engine->engineId(),engine);
				}
			}
		}

		if (replCount>0 && replCount+manualCount>1)
		{
			FStartReplicateTimer.stop();

			FWorker = new ReplicateWorker(replicationDatabaseConnection(),replicationDatabasePath(),this);
			connect(FWorker,SIGNAL(ready()),SLOT(onReplicateWorkerReady()));
			connect(FWorker,SIGNAL(finished()),SLOT(onReplicateWorkerFinished()));
			connect(FWorker,SIGNAL(taskFinished(ReplicateTask *)),SLOT(onReplicateWorkerTaskFinished(ReplicateTask *)));
			FWorker->start();
		}
		else foreach(const QUuid &engineId, FEngines.keys())
		{
			disconnectEngine(FEngines.take(engineId));
		}
	}
}

void ArchiveReplicator::onReplicateWorkerReady()
{
	startSyncModifications();
}

void ArchiveReplicator::onReplicateWorkerFinished()
{
	FLoadStateTasks.clear();
	FSaveModifsTasks.clear();
	FLoadModifsRequests.clear();
	FLoadModifsTasks.clear();
	FUpdateVersionTasks.clear();
	FLoadCollectionRequests.clear();
	FSaveCollectionRequests.clear();
	FRemoveCollectionRequests.clear();

	FSourceOrder.clear();
	FDestinations.clear();
	FModifications.clear();

	foreach(IArchiveEngine *engine, FConnectedEngines)
		disconnectEngine(engine);

	foreach(const QUuid &engineId, FEngines.keys())
		stopReplication(engineId);

	delete FWorker;
	FWorker = NULL;

	if (!FDestroy)
		FStartReplicateTimer.start(REPLICATE_RESTART_TIMEOUT);
	else
		deleteLater();
}

void ArchiveReplicator::onReplicateWorkerTaskFinished(ReplicateTask *ATask)
{
	switch(ATask->type())
	{
	case ReplicateTask::LoadState:
		{
			ReplicateTaskLoadState *task = static_cast<ReplicateTaskLoadState *>(ATask);
			IArchiveEngine *engine = FEngines.value(FLoadStateTasks.take(task->taskId()));
			if (!task->isFailed() && engine!=NULL)
			{
				if (engine->isCapable(FStreamJid,IArchiveEngine::ArchiveReplication))
				{
					QString requestId = engine->loadModifications(FStreamJid,task->startTime(),REPLICATE_MODIFICATIONS_COUNT,task->nextRef());
					if (!requestId.isEmpty())
					{
						FLoadModifsRequests.insert(requestId,engine->engineId());
					}
					else
					{
						stopReplication(engine->engineId());
						startSyncCollections();
					}
				}
				else
				{
					startSyncCollections();
				}
			}
			else if (engine != NULL)
			{
				stopReplication(engine->engineId());
				startSyncCollections();
			}
		}
		break;
	case ReplicateTask::SaveModifications:
		{
			ReplicateTaskSaveModifications *task = static_cast<ReplicateTaskSaveModifications *>(ATask);
			IArchiveEngine *engine = FEngines.value(FSaveModifsTasks.take(task->taskId()));
			if (!task->isFailed() && engine!=NULL)
			{
				IArchiveModifications modifications = task->modifications();
				if (!task->isCompleted())
				{
					QString requestId = engine->loadModifications(FStreamJid,modifications.start,REPLICATE_MODIFICATIONS_COUNT,modifications.next);
					if (!requestId.isEmpty())
					{
						FLoadModifsRequests.insert(requestId,engine->engineId());
					}
					else
					{
						stopReplication(engine->engineId());
						startSyncCollections();
					}
				}
				else
				{
					startSyncCollections();
				}
			}
			else if (engine != NULL)
			{
				stopReplication(engine->engineId());
				startSyncCollections();
			}
		}
		break;
	case ReplicateTask::LoadModifications:
		{
			ReplicateTaskLoadModifications *task = static_cast<ReplicateTaskLoadModifications *>(ATask);
			QList<QUuid> engines = FLoadModifsTasks.take(task->taskId());
			if (!task->isFailed() && !engines.isEmpty())
			{
				FModifications = task->modifications();
				QMultiMap<int, QUuid> loadOrder;
				foreach(IArchiveEngine *engine, FArchiver->archiveEngines())
				{
					if (FArchiver->isArchiveEngineEnabled(engine->engineId()))
					{
						int engineOrder = engine->capabilityOrder(IArchiveEngine::ArchiveManagement,FStreamJid);
						if (engineOrder > 0)
						{
							connectEngine(engine);
							loadOrder.insertMulti(engineOrder,engine->engineId());
						}
					}
				}
				FSourceOrder = loadOrder.values();

				startNextModification();
			}
			else foreach(const QUuid &engineId, engines)
			{
				stopReplication(engineId);
			}
		}
		break;
	case ReplicateTask::UpdateVersion:
		{
			ReplicateTaskUpdateVersion *task = static_cast<ReplicateTaskUpdateVersion *>(ATask);
			IArchiveEngine *engine = FEngines.value(FUpdateVersionTasks.take(task->taskId()));
			if (!task->isFailed() && engine!=NULL)
				FDestinations.removeAll(engine->engineId());
			else if (engine != NULL)
				stopReplication(engine->engineId());
			startNextModification();
		}
		break;
	}
	delete ATask;
}

void ArchiveReplicator::onEngineRequestFailed(const QString &AId, const XmppError &AError)
{
	Q_UNUSED(AError);
	if (FLoadModifsRequests.contains(AId))
	{
		QUuid engineId = FLoadModifsRequests.take(AId);
		stopReplication(engineId);
		startSyncCollections();
	}
	else if (FLoadCollectionRequests.contains(AId))
	{
		FLoadCollectionRequests.remove(AId);
		FDestinations.clear();
		startNextModification();
	}
	else if (FSaveCollectionRequests.contains(AId))
	{
		QUuid engineId = FSaveCollectionRequests.take(AId);
		FDestinations.removeAll(engineId);
		startNextModification();
	}
	else if (FRemoveCollectionRequests.contains(AId))
	{
		if (AError.condition() != "item-not-found")
		{
			QUuid engineId = FRemoveCollectionRequests.take(AId);
			FDestinations.removeAll(engineId);
			startNextModification();
		}
		else
		{
			// Item not exists == item deleted
			static const IArchiveRequest request;
			onEngineCollectionsRemoved(AId,request);
		}
	}
}

void ArchiveReplicator::onEngineCollectionSaved(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FSaveCollectionRequests.contains(AId))
	{
		QUuid engineId = FSaveCollectionRequests.take(AId);
		ReplicateTaskUpdateVersion *task = new ReplicateTaskUpdateVersion(engineId,FModification,ACollection.header.version);
		if (FWorker->startTask(task))
			FUpdateVersionTasks.insert(task->taskId(),engineId);
		else
			stopReplication(engineId);
	}
}

void ArchiveReplicator::onEngineCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FLoadCollectionRequests.contains(AId))
	{
		QUuid engineId =  FLoadCollectionRequests.take(AId);
		if (ACollection.header == FModification.header)
		{
			foreach(const QUuid &engineId, FDestinations)
			{
				IArchiveEngine *engine = FArchiver->findArchiveEngine(engineId);
				if (engine)
				{
					QString requestId = engine->saveCollection(FStreamJid,ACollection);
					if (!requestId.isEmpty())
						FSaveCollectionRequests.insert(requestId,engineId);
					else
						FDestinations.removeAll(engineId);
				}
				else
				{
					stopReplication(engineId);
				}
			}
		}
		else
		{
			FDestinations.clear();
		}
		startNextModification();
	}
}

void ArchiveReplicator::onEngineCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest)
{
	Q_UNUSED(ARequest);
	if (FRemoveCollectionRequests.contains(AId))
	{
		QUuid engineId = FRemoveCollectionRequests.take(AId);
		ReplicateTaskUpdateVersion *task = new ReplicateTaskUpdateVersion(engineId,FModification,0);
		if (FWorker->startTask(task))
			FUpdateVersionTasks.insert(task->taskId(),engineId);
		else
			stopReplication(engineId);
	}
}

void ArchiveReplicator::onEngineModificationsLoaded(const QString &AId, const IArchiveModifications &AModifications)
{
	if (FLoadModifsRequests.contains(AId))
	{
		IArchiveEngine *engine = FEngines.value(FLoadModifsRequests.take(AId));
		if (engine)
		{
			ReplicateTaskSaveModifications *task = new ReplicateTaskSaveModifications(engine->engineId(),AModifications,AModifications.items.isEmpty());
			if (FWorker->startTask(task))
			{
				FSaveModifsTasks.insert(task->taskId(),engine->engineId());
			}
			else
			{
				stopReplication(engine->engineId());
				startSyncCollections();
			}
		}
	}
}
