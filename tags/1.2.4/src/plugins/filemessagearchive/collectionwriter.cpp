#include "collectionwriter.h"

#define MIN_SIZE_CLOSE_TIMEOUT        120*60*1000
#define MAX_SIZE_CLOSE_TIMEOUT        60*1000
#define NORMAL_SIZE_CLOSE_TIMEOUT     20*60*1000
#define CRITICAL_SIZE_CLOSE_TIMEOUT   0

CollectionWriter::CollectionWriter(const Jid &AStreamJid, const QString &AFileName, const IArchiveHeader &AHeader, QObject *AParent) : QObject(AParent)
{
	FXmlFile = NULL;
	FXmlWriter = NULL;

	FGroupchat = false;
	FNotesCount = 0;
	FMessagesCount = 0;

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
	emit writerDestroyed(this);
}

bool CollectionWriter::isOpened() const
{
	return FXmlWriter!=NULL;
}

const Jid &CollectionWriter::streamJid() const
{
	return FStreamJid;
}

const QString &CollectionWriter::fileName() const
{
	return FFileName;
}

const IArchiveHeader &CollectionWriter::header() const
{
	return FHeader;
}

int CollectionWriter::recordsCount() const
{
	return FMessagesCount + FNotesCount;
}

bool CollectionWriter::writeMessage(const Message &AMessage, const QString &ASaveMode, bool ADirectionIn)
{
	if (isOpened() && ASaveMode != ARCHIVE_SAVE_FALSE)
	{
		Jid contactJid = AMessage.from();
		FGroupchat |= AMessage.type()==Message::GroupChat;
		if (!FGroupchat || !contactJid.resource().isEmpty())
		{
			FMessagesCount++;
			FXmlWriter->writeStartElement(ADirectionIn ? "from" : "to");

			int secs = FHeader.start.secsTo(AMessage.dateTime());
			if (secs >= 0)
				FXmlWriter->writeAttribute("secs",QString::number(secs));
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
	if (isOpened() && !ANote.isEmpty())
	{
		FNotesCount++;
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

void CollectionWriter::closeAndDeleteLater()
{
	stopCollection();
	deleteLater();
}

void CollectionWriter::startCollection()
{
	FXmlWriter->setAutoFormatting(true);
	FXmlWriter->writeStartElement("chat");
	FXmlWriter->writeAttribute("with",FHeader.with.full());
	FXmlWriter->writeAttribute("start",DateTime(FHeader.start).toX85UTC());
	FXmlWriter->writeAttribute("version",QString::number(FHeader.version));
	if (!FHeader.subject.isEmpty())
		FXmlWriter->writeAttribute("subject",FHeader.subject);
	if (!FHeader.threadId.isEmpty())
		FXmlWriter->writeAttribute("thread",FHeader.threadId);
	FXmlWriter->writeAttribute("secsFromLast","false");
	checkLimits();
}

void CollectionWriter::stopCollection()
{
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
		if (FMessagesCount == 0)
			QFile::remove(FFileName);
		FXmlFile->deleteLater();
		FXmlFile = NULL;
	}
}

void CollectionWriter::writeElementChilds(const QDomElement &AElem)
{
	QDomNode node = AElem.firstChild();
	while (!node.isNull())
	{
		if (node.isElement())
		{
			QDomElement elem = node.toElement();
			if (elem.tagName() != "thread")
			{
				FXmlWriter->writeStartElement(elem.nodeName());
				if (!elem.namespaceURI().isEmpty())
					FXmlWriter->writeAttribute("xmlns",elem.namespaceURI());

				QDomNamedNodeMap attrMap = elem.attributes();
				for (uint i =0; i<attrMap.length(); i++)
				{
					QDomNode attrNode = attrMap.item(i);
					FXmlWriter->writeAttribute(attrNode.nodeName(),attrNode.nodeValue());
				}

				writeElementChilds(elem);
				FXmlWriter->writeEndElement();
			}
		}
		else if (node.isCharacterData())
		{
			FXmlWriter->writeCharacters(node.toCharacterData().data());
		}

		node = node.nextSibling();
	}
}

void CollectionWriter::checkLimits()
{
	if (FXmlFile->size() > Options::node(OPV_FILEARCHIVE_COLLECTION_CRITICALSIZE).value().toInt())
		FCloseTimer.start(CRITICAL_SIZE_CLOSE_TIMEOUT);
	else if (FXmlFile->size() > Options::node(OPV_FILEARCHIVE_COLLECTION_MAXSIZE).value().toInt())
		FCloseTimer.start(MAX_SIZE_CLOSE_TIMEOUT);
	else if (FXmlFile->size() > Options::node(OPV_FILEARCHIVE_COLLECTION_MINSIZE).value().toInt())
		FCloseTimer.start(NORMAL_SIZE_CLOSE_TIMEOUT);
	else
		FCloseTimer.start(MIN_SIZE_CLOSE_TIMEOUT);
}
