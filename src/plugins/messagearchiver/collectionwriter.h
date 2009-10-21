#ifndef COLLECTIONWRITER_H
#define COLLECTIONWRITER_H

#include <QFile>
#include <QTimer>
#include <QXmlStreamWriter>
#include <definations/namespaces.h>
#include <interfaces/imessagearchiver.h>
#include <utils/message.h>
#include <utils/datetime.h>

class CollectionWriter : 
  public QObject
{
  Q_OBJECT;
public:
  CollectionWriter(const Jid &AStreamJid, const QString &AFileName, const IArchiveHeader &AHeader, QObject *AParent);
  ~CollectionWriter();
  bool isOpened() const;
  const Jid &streamJid() const { return FStreamJid; }
  const QString &fileName() const { return FFileName; }
  const IArchiveHeader &header() const { return FHeader; }
  int recordsCount() const { return FRecsCount; }
  int secondsFromStart() const { return FSecsSum; }
  bool writeMessage(const Message &AMessage, const QString &ASaveMode, bool ADirectionIn);
  bool writeNote(const QString &ANote);
  void close();
signals:
  void destroyed(const Jid &AStreamJid, CollectionWriter *AWriter);
protected:
  void startCollection();
  void stopCollection();
  void writeElementChilds(const QDomElement &AElem);
  void checkLimits();
private:
  QTimer FCloseTimer;
  QFile *FXmlFile;
  QXmlStreamWriter *FXmlWriter;
private:
  int FRecsCount;
  int FSecsSum;
  bool FGroupchat;
  Jid FStreamJid;
  QString FFileName;
  IArchiveHeader FHeader;
};

#endif // COLLECTIONWRITER_H
