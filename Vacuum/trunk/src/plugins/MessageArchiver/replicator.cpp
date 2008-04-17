#include "replicator.h"

#include <QFile>

#define START_INTERVAL            5*60*1000
#define STEP_INTERVAL             10*1000

#define REPLICATION_FILE_NAME     "replication.xml"
#define REPLICATION_TAG           "replication"
#define SERVER2LOCAL_TAG          "server2local"

#define MAX_MODIFICATIONS         5

Replicator::Replicator(IMessageArchiver *AArchiver, const Jid &AStreamJid, const QString &ADirPath, QObject *AParent) : QObject(AParent)
{
  FArchiver = AArchiver;
  FStreamJid = AStreamJid;
  FDirPath = ADirPath;

  FStartTimer.setSingleShot(true);
  connect(&FStartTimer,SIGNAL(timeout()),SLOT(onStartTimerTimeout()));

  FStepTimer.setSingleShot(true);
  connect(&FStepTimer,SIGNAL(timeout()),SLOT(onStepTimerTimeout()));

  connect(FArchiver->instance(),SIGNAL(serverCollectionLoaded(const QString &, const IArchiveCollection &, const QString &, int)),
    SLOT(onServerCollectionLoaded(const QString &, const IArchiveCollection &, const QString &, int )));
  connect(FArchiver->instance(),SIGNAL(serverModificationsLoaded(const QString &, const IArchiveModifications &, const QString &, int)),
    SLOT(onServerModificationsLoaded(const QString &, const IArchiveModifications &, const QString &, int)));
  connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
    SLOT(onRequestFailed(const QString &, const QString &)));

  FStartTimer.start(STEP_INTERVAL);
}

Replicator::~Replicator()
{

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
  FServerLast = s2lElem.attribute("last","1970-01-01T00:00:00Z");
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

  QDomElement s2lElem = rootElem.firstChildElement(SERVER2LOCAL_TAG);
  if (s2lElem.isNull())
    s2lElem = rootElem.appendChild(doc.createElement(SERVER2LOCAL_TAG)).toElement();
  s2lElem.setAttribute("last",FServerLast.toX85UTC());

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
  if (loadStatus())
    FServerRequest = FArchiver->loadServerModifications(FStreamJid,FServerLast.toLocal(),MAX_MODIFICATIONS);
  else
    FServerRequest.clear();
  if (FServerRequest.isEmpty())
    restart();
}

void Replicator::onStepTimerTimeout()
{
  bool accepted = false;
  while (!accepted && !FServerModifs.items.isEmpty())
  {
    IArchiveModification modif = FServerModifs.items.takeFirst();
    if (modif.action != IArchiveModification::Removed)
    {
      IArchiveHeader header = FArchiver->loadLocalHeader(FStreamJid,modif.header.with,modif.header.start);
      if (header!=modif.header || header.version < modif.header.version)
      {
        accepted = true;
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
  
  if (!accepted && FServerModifs.items.isEmpty())
  {
    FServerLast = FServerModifs.endTime;
    saveStatus();
    restart();
  }
}

void Replicator::onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ALast, int /*ACount*/)
{
  if (FServerRequest == AId)
  {
    FServerCollection.header = ACollection.header;
    FServerCollection.messages += ACollection.messages;
    FServerCollection.notes += ACollection.notes;
    if (!ACollection.messages.isEmpty() || !ACollection.notes.isEmpty())
    {
      FServerRequest = FArchiver->loadServerCollection(FStreamJid,ACollection.header,ALast);
      if (FServerRequest.isEmpty())
        restart();
    }
    else if (!FServerCollection.messages.isEmpty() || !FServerCollection.notes.isEmpty())
    {
      FArchiver->saveLocalCollection(FStreamJid,FServerCollection,false);
      nextStep();
    }
    else
      nextStep();
  }
}

void Replicator::onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const QString &/*ALast*/, int /*ACount*/)
{
  if (FServerRequest == AId)
  {
    FServerModifs.startTime = AModifs.startTime;
    FServerModifs.endTime = AModifs.endTime;
    FServerModifs.items.clear();

    if (!AModifs.items.isEmpty())
    {
      foreach(IArchiveModification modif, AModifs.items)
      {
        if (modif.action == IArchiveModification::Created)
        {
          FServerModifs.items.append(modif);
        }
        else if (modif.action == IArchiveModification::Modified)
        {
          FServerModifs.items.append(modif);
        }
        else if (modif.action == IArchiveModification::Removed)
        {
          FServerModifs.items.append(modif);
        }
      }
    }

    if (!FServerModifs.items.isEmpty())
      nextStep();
    else
      restart();
  }
}

void Replicator::onRequestFailed(const QString &AId, const QString &/*AError*/)
{
  if (FServerRequest == AId)
  {
    restart();
  }
}

