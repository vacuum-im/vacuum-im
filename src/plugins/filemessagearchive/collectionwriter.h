#ifndef COLLECTIONWRITER_H
#define COLLECTIONWRITER_H

#include <QFile>
#include <QTimer>
#include <QXmlStreamWriter>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <interfaces/imessagearchiver.h>
#include <utils/message.h>
#include <utils/datetime.h>
#include <utils/options.h>

class CollectionWriter :
			public QObject
{
	Q_OBJECT;
public:
	CollectionWriter(const Jid &AStreamJid, const QString &AFileName, const IArchiveHeader &AHeader, QObject *AParent);
	~CollectionWriter();
	bool isOpened() const;
	const Jid &streamJid() const;
	const QString &fileName() const;
	const IArchiveHeader &header() const;
	int recordsCount() const;
	bool writeMessage(const Message &AMessage, const QString &ASaveMode, bool ADirectionIn);
	bool writeNote(const QString &ANote);
	void closeAndDeleteLater();
signals:
	void writerDestroyed(CollectionWriter *AWriter);
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
	bool FGroupchat;
	int FNotesCount;
	int FMessagesCount;
	Jid FStreamJid;
	QString FFileName;
	IArchiveHeader FHeader;
};

#endif // COLLECTIONWRITER_H
