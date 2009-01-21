#ifndef REPLICATOR_H
#define REPLICATOR_H

#include <QQueue>
#include <QTimer>
#include "../../interfaces/imessagearchiver.h"
#include "../../utils/datetime.h"

class Replicator : 
  public QObject
{
  Q_OBJECT;
public:
  Replicator(IMessageArchiver *AArchiver, const Jid &AStreamJid, const QString &ADirPath, QObject *AParent);
  ~Replicator();
  Jid streamJid() const;
  QDateTime replicationPoint() const;
  bool enabled() const;
  void setEnabled(bool AEnabled);
protected:
  bool loadStatus();
  bool saveStatus();
  void restart();
  void nextStep();
protected slots:
  void onStartTimerTimeout();
  void onStepTimerTimeout();
  void onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult);
  void onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult);
  void onRequestFailed(const QString &AId, const QString &AError);
private:
  IMessageArchiver *FArchiver;
private:
  QTimer FStartTimer;
  QTimer FStepTimer;
private:
  QString FReplicationLast;
  DateTime FReplicationStart;
  DateTime FReplicationPoint;
  QString FServerRequest;
  IArchiveModifications FServerModifs;
  IArchiveCollection FServerCollection;
private:
  bool FEnabled;
  Jid FStreamJid;
  QString FDirPath;
};

#endif // REPLICATOR_H
