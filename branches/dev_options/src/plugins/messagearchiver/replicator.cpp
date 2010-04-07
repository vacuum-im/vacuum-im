#include "replicator.h"

#include <QFile>

#define MAX_MODIFICATIONS         5

#define START_INTERVAL            5*60*1000
#define STEP_INTERVAL             10*1000

#define REPLICATION_FILE_NAME     "replication.xml"
#define REPLICATION_TAG           "replication"
#define SERVER2LOCAL_TAG          "server2local"

Replicator::Replicator(IMessageArchiver *AArchiver, const Jid &AStreamJid, const QString &ADirPath, QObject *AParent) : QObject(AParent)
{
  FArchiver = AArchiver;
  FStreamJid = AStreamJid;
  FDirPath = ADirPath;
  FEnabled = false;

  FStartTimer.setSingleShot(true);
  connect(&FStartTimer,SIGNAL(timeout()),SLOT(onStartTimerTimeout()));

  FStepTimer.setSingleShot(true);
  connect(&FStepTimer,SIGNAL(timeout()),SLOT(onStepTimerTimeout()));

  connect(FArchiver->instance(),SIGNAL(serverCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)),
    SLOT(onServerCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)));
  connect(FArchiver->instance(),SIGNAL(serverModificationsLoaded(const QString &, const IArchiveModifications &, const IArchiveResultSet &)),
    SLOT(onServerModificationsLoaded(const QString &, const IArchiveModifications &, const IArchiveResultSet &)));
  connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
    SLOT(onRequestFailed(const QString &, const QString &)));

  if (loadStatus())
  {
    FReplicationLast = "";
    FReplicationStart = FReplicationPoint.isValid() ? FReplicationPoint : DateTime("1970-01-01T00:00:00Z");
    setEnabled(true);
  }
}

Replicator::~Replicator()
{

}

Jid Replicator::streamJid() const
{
  return FStreamJid;
}

QDateTime Replicator::replicationPoint() const
{
  return FReplicationPoint.toLocal();
}

bool Replicator::enabled() const
{
  return FEnabled;
}

void Replicator::setEnabled(bool AEnabled)
{
  if (FEnabled != AEnabled)
  {
    if (!AEnabled)
    {
      FStartTimer.stop();
      FStepTimer.stop();
      FServerRequest.clear();
    }
    else
    {
      FStartTimer.start(START_INTERVAL);
    }
    FEnabled = AEnabled;
  }
}

bool Replicator::loadStatus()
{
  QDomDocument doc;
  QFile file(FDirPath+"/"+REPLICATION_FILE_NAME);
  if (file.open(QFile::ReadOnly))
  {
    doc.setContent(file.readAll(),true);
    file.close();
  }
  else if (file.open(QFile::WriteOnly|QFile::Truncate))
  {
    doc.appendChild(doc.createElement(REPLICATION_TAG)).toElement().setAttribute("version","1.0");
    file.write(doc.toByteArray(2));
    file.close();
  }
  else
    return false;

  QDomElement s2lElem = doc.documentElement().firstChildElement(SERVER2LOCAL_TAG);
  FReplicationPoint = s2lElem.attribute("point");
  return true;
}

bool Replicator::saveStatus()
{
  QDomDocument doc;
  QFile file(FDirPath+"/"+REPLICATION_FILE_NAME);
  if (file.open(QFile::ReadOnly))
  {
    doc.setContent(file.readAll(),true);
    file.close();
  }

  QDomElement rootElem = doc.documentElement();
  if (rootElem.tagName()!=REPLICATION_TAG)
  {
    doc.clear();
    rootElem = doc.appendChild(doc.createElement(REPLICATION_TAG)).toElement();
    rootElem.setAttribute("version","1.0");
  }

  if (FReplicationPoint.isValid())
  {
    QDomElement s2lElem = rootElem.firstChildElement(SERVER2LOCAL_TAG);
    if (s2lElem.isNull())
      s2lElem = rootElem.appendChild(doc.createElement(SERVER2LOCAL_TAG)).toElement();
    s2lElem.setAttribute("point",FReplicationPoint.toX85UTC());
  }

  if (file.open(QFile::WriteOnly|QFile::Truncate))
  {
    file.write(doc.toByteArray(2));
    file.close();
    return true;
  }
  return false;
}

void Replicator::restart()
{
  FStartTimer.start(START_INTERVAL);
}

void Replicator::nextStep()
{
  FStepTimer.start(STEP_INTERVAL);
}

void Replicator::onStartTimerTimeout()
{
  FServerRequest = FArchiver->loadServerModifications(FStreamJid,FReplicationStart.toLocal(),MAX_MODIFICATIONS,FReplicationLast);
  if (FServerRequest.isEmpty())
    restart();
}

void Replicator::onStepTimerTimeout()
{
  bool loading = false;
  while (!loading && !FServerModifs.items.isEmpty())
  {
    IArchiveModification modif = FServerModifs.items.takeFirst();
    if (modif.action != IArchiveModification::Removed)
    {
      IArchiveHeader header = FArchiver->loadLocalHeader(FStreamJid,modif.header.with,modif.header.start);
      //if (header!=modif.header || header.version < modif.header.version)
      {
        loading = true;
        FServerRequest = FArchiver->loadServerCollection(FStreamJid,modif.header);
        if (!FServerRequest.isEmpty())
        {
          FServerCollection.header = modif.header;
          FServerCollection.messages.clear();
        }
        else
          restart();
      }
    }
    else
    {
      FArchiver->removeLocalCollection(FStreamJid,modif.header);
    }
  }
  
  if (!loading && FServerModifs.items.isEmpty())
  {
    saveStatus();
    restart();
  }
}

void Replicator::onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult)
{
  if (FServerRequest == AId)
  {
    FServerCollection.header = ACollection.header;
    FServerCollection.messages += ACollection.messages;
    FServerCollection.notes += ACollection.notes;
    if (!AResult.last.isEmpty() && AResult.index+ACollection.messages.count()+ACollection.notes.count()<AResult.count)
    {
      FServerRequest = FArchiver->loadServerCollection(FStreamJid,ACollection.header,AResult.last);
      if (FServerRequest.isEmpty())
        restart();
    }
    else if (!FServerCollection.messages.isEmpty() || !FServerCollection.notes.isEmpty())
    {
      FArchiver->saveLocalCollection(FStreamJid,FServerCollection,false);
      nextStep();
    }
    else
    {
      nextStep();
    }
  }
}

void Replicator::onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult)
{
  if (FServerRequest == AId)
  {
    FServerModifs = AModifs;
    if (!FServerModifs.items.isEmpty())
    {
      FReplicationLast = AResult.last;
      FReplicationPoint = AModifs.endTime;
      nextStep();
    }
    else
    {
      FReplicationPoint = QDateTime::currentDateTime();
      restart();
    }
  }
}

void Replicator::onRequestFailed(const QString &AId, const QString &/*AError*/)
{
  if (FServerRequest == AId)
  {
    restart();
  }
}

