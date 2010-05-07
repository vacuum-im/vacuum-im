#include "filestreamswindow.h"

#include <QTimer>
#include <QVariant>

#define UPDATE_STATUSBAR_INTERVAL   500

enum StreamColumns {
	CMN_FILENAME,
	CMN_STATE,
	CMN_SIZE,
	CMN_PROGRESS,
	CMN_SPEED,
	CMN_COUNT
};

enum ItemDataRoles {
	IDR_VALUE       = Qt::UserRole + 1,
	IDR_STREAMID
};

FileStreamsWindow::FileStreamsWindow(IFileStreamsManager *AManager, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

	FManager = AManager;

	setWindowTitle(tr("File Transfers"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_FILESTREAMSMANAGER,0,0,"windowIcon");

	FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);
	FStatusBarChanger = new StatusBarChanger(ui.stbStatusBar);

	FProxy.setSourceModel(&FStreamsModel);
	FProxy.setDynamicSortFilter(true);
	FProxy.setSortCaseSensitivity(Qt::CaseInsensitive);
	FProxy.setSortLocaleAware(true);

	ui.tbvStreams->setModel(&FProxy);
	ui.tbvStreams->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	connect(ui.tbvStreams,SIGNAL(activated(const QModelIndex &)),SLOT(onTableIndexActivated(const QModelIndex &)));

	connect(FManager->instance(),SIGNAL(streamCreated(IFileStream *)),SLOT(onStreamCreated(IFileStream *)));
	connect(FManager->instance(),SIGNAL(streamDestroyed(IFileStream *)),SLOT(onStreamDestroyed(IFileStream *)));

	initialize();
}

FileStreamsWindow::~FileStreamsWindow()
{
	Options::setFileValue(saveGeometry(),"filestreams.filestreamswindow.geometry");
	Options::setFileValue(saveState(),"filestreams.filestreamswindow.state");
}

void FileStreamsWindow::initialize()
{
	restoreGeometry(Options::fileValue("filestreams.filestreamswindow.geometry").toByteArray());
	restoreState(Options::fileValue("filestreams.filestreamswindow.state").toByteArray());

	FStreamsModel.setColumnCount(CMN_COUNT);
	FStreamsModel.setHorizontalHeaderLabels(QStringList()<<tr("File Name")<<tr("State")<<tr("Size")<<tr("Progress")<<tr("Speed"));

	for (int column=0; column<CMN_COUNT; column++)
		ui.tbvStreams->horizontalHeader()->setResizeMode(column,column!=CMN_FILENAME ? QHeaderView::ResizeToContents : QHeaderView::Stretch);

	foreach(IFileStream *stream, FManager->streams()) {
		appendStream(stream);
	}

	FProxy.setSortRole(IDR_VALUE);
	ui.tbvStreams->horizontalHeader()->setSortIndicator(CMN_FILENAME,Qt::AscendingOrder);

	FStreamsCount = new QLabel(ui.stbStatusBar);
	FStreamsSpeedIn = new QLabel(ui.stbStatusBar);
	FStreamsSpeedOut = new QLabel(ui.stbStatusBar);
	FStatusBarChanger->insertWidget(FStreamsCount,SBG_FSMW_STREAMS_COUNT);
	FStatusBarChanger->insertWidget(FStreamsSpeedIn,SBG_FSMW_STREAMS_SPPED_IN);
	FStatusBarChanger->insertWidget(FStreamsSpeedOut,SBG_FSMW_STREAMS_SPEED_OUT);
	onUpdateStatusBar();
}

void FileStreamsWindow::appendStream(IFileStream *AStream)
{
	if (streamRow(AStream->streamId()) < 0)
	{
		QList<QStandardItem *> columns;
		QVariant sid = AStream->streamId();
		for (int column=0; column<CMN_COUNT; column++)
		{
			columns.append(new QStandardItem());
			columns[column]->setData(sid, IDR_STREAMID);
			columns[column]->setTextAlignment(column!=CMN_FILENAME ? Qt::AlignCenter : Qt::AlignVCenter|Qt::AlignLeft);
		}
		if (AStream->streamKind() == IFileStream::SendFile)
			columns[CMN_FILENAME]->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_FILETRANSFER_SEND));
		else
			columns[CMN_FILENAME]->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_FILETRANSFER_RECEIVE));
		FStreamsModel.appendRow(columns);

		connect(AStream->instance(), SIGNAL(stateChanged()),SLOT(onStreamStateChanged()));
		connect(AStream->instance(), SIGNAL(speedChanged()),SLOT(onStreamSpeedChanged()));
		connect(AStream->instance(), SIGNAL(progressChanged()),SLOT(onStreamProgressChanged()));
		connect(AStream->instance(), SIGNAL(propertiesChanged()),SLOT(onStreamPropertiesChanged()));

		updateStreamState(AStream);
		updateStreamSpeed(AStream);
		updateStreamProgress(AStream);
		updateStreamProperties(AStream);
	}
}

void FileStreamsWindow::updateStreamState(IFileStream *AStream)
{
	QList<QStandardItem *> columns = streamColumns(AStream->streamId());
	if (!columns.isEmpty())
	{
		switch (AStream->streamState())
		{
		case IFileStream::Creating:
			columns[CMN_STATE]->setText(tr("Create"));
			break;
		case IFileStream::Negotiating:
			columns[CMN_STATE]->setText(tr("Negotiate"));
			break;
		case IFileStream::Connecting:
			columns[CMN_STATE]->setText(tr("Connect"));
			break;
		case IFileStream::Transfering:
			columns[CMN_STATE]->setText(tr("Transfer"));
			break;
		case IFileStream::Disconnecting:
			columns[CMN_STATE]->setText(tr("Disconnect"));
			break;
		case IFileStream::Finished:
			columns[CMN_STATE]->setText(tr("Finished"));
			break;
		case IFileStream::Aborted:
			columns[CMN_STATE]->setText(tr("Aborted"));
			break;
		default:
			columns[CMN_STATE]->setText(tr("Unknown"));
		}
		columns[CMN_STATE]->setData(AStream->streamState(), IDR_VALUE);
	}
}

void FileStreamsWindow::updateStreamSpeed(IFileStream *AStream)
{
	QList<QStandardItem *> columns = streamColumns(AStream->streamId());
	if (!columns.isEmpty())
	{
		columns[CMN_SPEED]->setText(sizeName(AStream->speed())+tr("/sec"));
		columns[CMN_SPEED]->setData(AStream->speed(), IDR_VALUE);
	}
}

void FileStreamsWindow::updateStreamProgress(IFileStream *AStream)
{
	QList<QStandardItem *> columns = streamColumns(AStream->streamId());
	if (!columns.isEmpty())
	{
		qint64 minPos = AStream->rangeOffset();
		qint64 maxPos = AStream->rangeLength()>0 ? AStream->rangeLength()+AStream->rangeOffset() : AStream->fileSize();
		qint64 percent = maxPos>0 ? ((minPos+AStream->progress())*100)/maxPos : 0;
		columns[CMN_PROGRESS]->setText(QString::number(percent)+"%");
		columns[CMN_PROGRESS]->setData(percent, IDR_VALUE);
	}
}

void FileStreamsWindow::updateStreamProperties(IFileStream *AStream)
{
	QList<QStandardItem *> columns = streamColumns(AStream->streamId());
	if (!columns.isEmpty())
	{
		QString fname = !AStream->fileName().isEmpty() ? AStream->fileName().split("/").last() : QString::null;
		columns[CMN_FILENAME]->setText(fname);
		columns[CMN_FILENAME]->setData(fname, IDR_VALUE);

		columns[CMN_SIZE]->setText(sizeName(AStream->fileSize()));
		columns[CMN_SIZE]->setData(AStream->fileSize(), IDR_VALUE);
	}
}

void FileStreamsWindow::removeStream(IFileStream *AStream)
{
	int row = streamRow(AStream->streamId());
	if (row >= 0)
		qDeleteAll(FStreamsModel.takeRow(row));
}

int FileStreamsWindow::streamRow(const QString &AStreamId) const
{
	for (int row=0; row<FStreamsModel.rowCount(); row++)
		if (FStreamsModel.item(row,CMN_FILENAME)->data(IDR_STREAMID).toString() == AStreamId)
			return row;
	return -1;
}

QList<QStandardItem *> FileStreamsWindow::streamColumns(const QString &AStreamId) const
{
	QList<QStandardItem *> columns;
	int row = streamRow(AStreamId);
	if (row >= 0)
		for (int column=0; column<CMN_COUNT; column++)
			columns.append(FStreamsModel.item(row,column));
	return columns;
}

QString FileStreamsWindow::sizeName(qint64 ABytes) const
{
	static int md[] = {1, 10, 100};
	QString units = tr("B","Byte");
	qreal value = ABytes;

	if (value > 1024)
	{
		value = value / 1024;
		units = tr("KB","Kilobyte");
	}
	if (value > 1024)
	{
		value = value / 1024;
		units = tr("MB","Megabyte");
	}
	if (value > 1024)
	{
		value = value / 1024;
		units = tr("GB","Gigabyte");
	}

	int prec = 0;
	if (value < 10)
		prec = 2;
	else if (value < 100)
		prec = 1;

	while (prec>0 && (qreal)qRound64(value*md[prec-1])/md[prec-1]==(qreal)qRound64(value*md[prec])/md[prec])
		prec--;

	value = (qreal)qRound64(value*md[prec])/md[prec];

	return QString::number(value,'f',prec)+units;
}

void FileStreamsWindow::onStreamCreated(IFileStream *AStream)
{
	appendStream(AStream);
}

void FileStreamsWindow::onStreamStateChanged()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
		updateStreamState(stream);
}

void FileStreamsWindow::onStreamSpeedChanged()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
		updateStreamSpeed(stream);
}

void FileStreamsWindow::onStreamProgressChanged()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
		updateStreamProgress(stream);
}

void FileStreamsWindow::onStreamPropertiesChanged()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
		updateStreamProperties(stream);
}

void FileStreamsWindow::onStreamDestroyed(IFileStream *AStream)
{
	removeStream(AStream);
}

void FileStreamsWindow::onTableIndexActivated(const QModelIndex &AIndex)
{
	QString streamId = AIndex.data(IDR_STREAMID).toString();
	IFileStreamsHandler *handler = FManager->streamHandler(streamId);
	if (handler)
		handler->fileStreamShowDialog(streamId);
}

void FileStreamsWindow::onUpdateStatusBar()
{
	int sendStreams=0, receiveStreams=0, countStreams=0;
	int speedIn=0, speedOut=0;

	foreach (IFileStream *stream, FManager->streams())
	{
		if (stream->streamState() == IFileStream::Transfering)
		{
			if (stream->streamKind() == IFileStream::SendFile)
			{
				sendStreams++;
				speedOut += stream->speed();
			}
			else
			{
				receiveStreams++;
				speedIn += stream->speed();
			}
		}
		countStreams++;
	}

	FStreamsCount->setText(tr("Active: %1/%2").arg(sendStreams+receiveStreams).arg(countStreams));
	FStreamsSpeedIn->setText(tr("Downloads: %1 at %2").arg(receiveStreams).arg(sizeName(speedIn)+tr("/sec")));
	FStreamsSpeedOut->setText(tr("Uploads: %1 at %2").arg(sendStreams).arg(sizeName(speedOut)+tr("/sec")));

	FStreamsCount->setMinimumWidth(qMax(FStreamsCount->minimumWidth(),FStreamsCount->sizeHint().width()));
	FStreamsSpeedIn->setMinimumWidth(qMax(FStreamsSpeedIn->minimumWidth(),FStreamsSpeedIn->sizeHint().width()));
	FStreamsSpeedOut->setMinimumWidth(qMax(FStreamsSpeedOut->minimumWidth(),FStreamsSpeedOut->sizeHint().width()));

	QTimer::singleShot(UPDATE_STATUSBAR_INTERVAL,this,SLOT(onUpdateStatusBar()));
}
