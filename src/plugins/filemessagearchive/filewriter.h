#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <QFile>
#include <QTimer>
#include <QXmlStreamWriter>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <interfaces/imessagearchiver.h>
#include <utils/message.h>
#include <utils/datetime.h>
#include <utils/options.h>

class FileWriter :
	public QObject
{
	Q_OBJECT;
public:
	FileWriter(const Jid &AStreamJid, const QString &AFileName, const IArchiveHeader &AHeader, QObject *AParent);
	~FileWriter();
	bool isOpened() const;
	const Jid &streamJid() const;
	const QString &fileName() const;
	const IArchiveHeader &header() const;
	int notesCount() const;
	int messagesCount() const;
	int recordsCount() const;
	int secondsFromStart() const;
	bool writeMessage(const Message &AMessage, const QString &ASaveMode, bool ADirectionIn);
	bool writeNote(const QString &ANote);
	void closeAndDeleteLater();
signals:
	void writerDestroyed(FileWriter *AWriter);
protected:
	void startCollection();
	void stopCollection();
	void writeElementChilds(const QDomElement &AElem);
	void checkLimits();
private:
	QFile *FXmlFile;
	QTimer FCloseTimer;
	QXmlStreamWriter *FXmlWriter;
private:
	int FSecsSum;
	bool FGroupchat;
	int FNotesCount;
	int FMessagesCount;
	Jid FStreamJid;
	QString FFileName;
	IArchiveHeader FHeader;
};

#endif // FILEWRITER_H
