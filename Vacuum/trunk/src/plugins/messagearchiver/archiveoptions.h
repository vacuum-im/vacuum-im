#ifndef ARCHIVEOPTIONS_H
#define ARCHIVEOPTIONS_H

#include "../../interfaces/imessagearchiver.h"
#include "ui_archiveoptions.h"

class ArchiveOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  ArchiveOptions(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent);
  ~ArchiveOptions();
  const Jid &streamJid() const { return FStreamJid; }
public slots:
  void apply();
  void reset();
protected slots:
  void onArchiveAutoSaveChanged(const Jid &AStreamJid, bool AAutoSave);
  void onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
  void onArchiveItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IArchiveItemPrefs &APrefs);
  void onArchiveRequestCompleted(const QString &AId);
  void onArchiveRequestFailed(const QString &AId, const QString &AError);
private:
  Ui::ArchiveOptionsClass ui;
private:
  IMessageArchiver *FArchiver;
private:
  Jid FStreamJid;
  QList<QString> FSaveRequests;
  QHash<Jid,QTableWidgetItem *> FTableItems;
};

#endif // ARCHIVEOPTIONS_H
