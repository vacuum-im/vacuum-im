#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <QFile>
#include <definitions/namespaces.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <utils/jid.h>
#include "transferthread.h"

#define SPEED_POINTS      10
#define SPEED_INTERVAL    500

class FileStream :
			public QObject,
			public IFileStream
{
	Q_OBJECT;
	Q_INTERFACES(IFileStream);
public:
	FileStream(IDataStreamsManager *ADataManager, const QString &AStreamId, const Jid &AStreamJid,
	           const Jid &AContactJid, int AKind, QObject *AParent);
	~FileStream();
	virtual QObject *instance() { return this; }
	virtual QString streamId() const;
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual int streamKind() const;
	virtual int streamState() const;
	virtual QString methodNS() const;
	virtual qint64 speed() const;
	virtual qint64 progress() const;
	virtual QString stateString() const;
	virtual bool isRangeSupported() const;
	virtual void setRangeSupported(bool ASupported);
	virtual qint64 rangeOffset() const;
	virtual void setRangeOffset(qint64 AOffset);
	virtual qint64 rangeLength() const;
	virtual void setRangeLength(qint64 ALength);
	virtual QString fileName () const;
	virtual void setFileName(const QString &AFileName);
	virtual qint64 fileSize() const;
	virtual void setFileSize(qint64 AFileSize);
	virtual QString fileHash() const;
	virtual void setFileHash(const QString &AFileHash);
	virtual QDateTime fileDate() const;
	virtual void setFileDate(const QDateTime &ADate);
	virtual QString fileDescription() const;
	virtual void setFileDescription(const QString &AFileDesc);
	virtual QUuid settingsProfile() const;
	virtual void setSettingsProfile(const QUuid &AProfileId);
	virtual QStringList acceptableMethods() const;
	virtual void setAcceptableMethods(const QStringList &AMethods);
	virtual bool initStream(const QList<QString> &AMethods);
	virtual bool startStream(const QString &AMethodNS);
	virtual void abortStream(const QString &AError);
signals:
	void stateChanged();
	void speedChanged();
	void progressChanged();
	void propertiesChanged();
	void streamDestroyed();
protected:
	bool openFile();
	bool updateFileInfo();
	void setStreamState(int AState, const QString &AMessage);
protected slots:
	void onSocketStateChanged(int AState);
	void onTransferThreadProgress(qint64 ABytes);
	void onTransferThreadFinished();
	void onIncrementSpeedIndex();
	void onConnectionTimeout();
private:
	IDataStreamsManager *FDataManager;
private:
	QString FStreamId;
	Jid FStreamJid;
	Jid FContactJid;
	int FStreamKind;
	int FStreamState;
	int FSpeedIndex;
	qint64 FSpeed[SPEED_POINTS];
	qint64 FProgress;
	bool FAborted;
	QUuid FProfileId;
	QString FAbortString;
	QString FStateString;
	QStringList FAcceptableMethods;
private:
	bool FRangeSupported;
	qint64 FRangeOffset;
	qint64 FRangeLength;
	qint64 FFileSize;
	QString FFileName;
	QString FFileDesc;
	QString FFileHash;
	QDateTime FFileDate;
private:
	QFile FFile;
	TransferThread *FThread;
	IDataStreamSocket *FSocket;
};

#endif // FILESTREAM_H
