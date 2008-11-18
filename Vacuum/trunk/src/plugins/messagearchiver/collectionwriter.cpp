#include "collectionwriter.h"

#define LIMIT_FILE_SIZE         7*1024
#define MAX_FILE_SIZE           10*1024
#define LIMIT_WAIT_TIME         1*60*100
#define MAX_WAIT_TIME           5*60*1000

CollectionWriter::CollectionWriter(const Jid &AStreamJid, const QString &AFileName, const IArchiveHeader &AHeader, QObject *AParent) : QObject(AParent)
{
  FXmlFile = NULL;
  FXmlWriter = NULL;

  FRecsCount = 0;
  FSecsSum = 0;
  FGroupchat = false;

  FStreamJid = AStreamJid;
  FFileName = AFileName;
  FHeader = AHeader;

  FCloseTimer.setSingleShot(true);
  connect(&FCloseTimer,SIGNAL(timeout()),SLOT(deleteLater()));

  if (!QFile::exists(FFileName))
  {
    FXmlFile = new QFile(FFileName,this);
    if (FXmlFile->open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
      FXmlWriter = new QXmlStreamWriter(FXmlFile);
      startCollection();
    }
  }

  if (!FXmlWriter)
    deleteLater();
}

CollectionWriter::~CollectionWriter()
{
  stopCollection();
  emit destroyed(FStreamJid, this);
}

bool CollectionWriter::isOpened() const
{
  return FXmlWriter!=NULL;
}

bool CollectionWriter::writeMessage(const Message &AMessage, const QString &ASaveMode, bool ADirectionIn)
{
  if (isOpened() && ASaveMode != ARCHIVE_SAVE_FALSE)
  {
    Jid contactJid = AMessage.from();
    FGroupchat |= AMessage.type()==Message::GroupChat;
    if (!FGroupchat || !contactJid.resource().isEmpty())
    {
      FRecsCount++;
      FCloseTimer.stop();

      FXmlWriter->writeStartElement(ADirectionIn ? "from" : "to");
      
      int secs = FHeader.start.secsTo(AMessage.dateTime());
      if (secs >= FSecsSum)
      {
        FXmlWriter->writeAttribute("secs",QString::number(secs-FSecsSum));
        FSecsSum += secs-FSecsSum;
      }
      else
        FXmlWriter->writeAttribute("utc",DateTime(AMessage.dateTime()).toX85UTC());

      if (FGroupchat)
        FXmlWriter->writeAttribute("name",contactJid.resource());
    
      if (ASaveMode == ARCHIVE_SAVE_BODY)
        FXmlWriter->writeTextElement("body",AMessage.body());
      else
        writeElementChilds(AMessage.stanza().document().documentElement());
      
      FXmlWriter->writeEndElement();
      FXmlFile->flush();

      checkLimits();
      return true;
    }
  }
  return false;
}

bool CollectionWriter::writeNote(const QString &ANote)
{
  if (isOpened() && ANote.isEmpty())
  {
    FRecsCount++;
    FCloseTimer.stop();
    FXmlWriter->writeStartElement("note");
    FXmlWriter->writeAttribute("utc",DateTime(QDateTime::currentDateTime()).toX85UTC());
    FXmlWriter->writeCharacters(ANote);
    FXmlWriter->writeEndElement();
    FXmlFile->flush();
    checkLimits();
    return true;
  }
  return false;
}

void CollectionWriter::close()
{
  stopCollection();
}

void CollectionWriter::startCollection()
{
  FCloseTimer.stop();
  FXmlWriter->setAutoFormatting(true);
  FXmlWriter->writeStartElement("chat");
  FXmlWriter->writeAttribute("with",FHeader.with.eFull());
  FXmlWriter->writeAttribute("start",DateTime(FHeader.start).toX85UTC());
  FXmlWriter->writeAttribute("version",QString::number(FHeader.version));
  if (!FHeader.subject.isEmpty())
    FXmlWriter->writeAttribute("subject",FHeader.subject);
  if (!FHeader.threadId.isEmpty())
    FXmlWriter->writeAttribute("thread",FHeader.threadId);
  checkLimits();
}

void CollectionWriter::stopCollection()
{
  FCloseTimer.stop();
  if (FXmlWriter)
  {
    FXmlWriter->writeEndElement();
    FXmlWriter->writeEndDocument();
    delete FXmlWriter;
    FXmlWriter = NULL;
  }
  if (FXmlFile)
  {
    FXmlFile->close();
    delete FXmlFile;
    FXmlFile = NULL;
  }
  if (FRecsCount == 0)
  {
    QFile::remove(FFileName);
  }
  deleteLater();
}

void CollectionWriter::writeElementChilds(const QDomElement &AElem)
{
  QDomElement elem = AElem.firstChildElement();
  while (!elem.isNull())
  {
    FXmlWriter->writeStartElement(elem.nodeName());
    if (!elem.namespaceURI().isEmpty())
      FXmlWriter->writeAttribute("xmlns",elem.namespaceURI());
    
    QDomNamedNodeMap map = elem.attributes();
    for (uint i =0; i<map.length(); i++)
    {
      QDomNode attrNode = map.item(i);
      FXmlWriter->writeAttribute(attrNode.nodeName(),attrNode.nodeValue());
    }

    if (!elem.text().isEmpty())
      FXmlWriter->writeCharacters(elem.text());

    writeElementChilds(elem);
    FXmlWriter->writeEndElement();

    elem = elem.nextSiblingElement();
  }
}

void CollectionWriter::checkLimits()
{
  int timeout = MAX_WAIT_TIME;
  if (FXmlFile->size() > LIMIT_FILE_SIZE)
    timeout = LIMIT_WAIT_TIME;
  else if (FXmlFile->size() > MAX_FILE_SIZE)
    timeout = 0;
  FCloseTimer.start(timeout);
}
