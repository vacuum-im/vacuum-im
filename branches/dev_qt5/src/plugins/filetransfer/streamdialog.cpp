#include "streamdialog.h"

#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDesktopServices>

StreamDialog::StreamDialog(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, IFileTransfer *AFileTransfer, IFileStream *AFileStream, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.wdtMethods->setLayout(new QVBoxLayout);
	ui.wdtMethods->layout()->setMargin(0);

	FFileStream = AFileStream;
	FFileTransfer = AFileTransfer;
	FFileManager = AFileManager;
	FDataManager = ADataManager;

	ui.pgbPrgress->setMinimum(0);
	ui.pgbPrgress->setMaximum(100);

	if (FFileStream->streamKind() == IFileStream::SendFile)
	{
		setWindowTitle(tr("Send File - %1").arg(FFileStream->streamJid().uFull()));
		ui.lblContactLabel->setText(tr("To:"));
	}
	else
	{
		setWindowTitle(tr("Receive File - %1").arg(FFileStream->streamJid().uFull()));
		ui.lblContactLabel->setText(tr("From:"));
	}

	ui.lblContact->setText(FFileStream->contactJid().uFull().toHtmlEscaped());

	if (AFileStream->streamState() == IFileStream::Creating)
	{
		foreach(QUuid profileId, FDataManager->settingsProfiles())
			ui.cmbSettingsProfile->addItem(FDataManager->settingsProfileName(profileId), profileId.toString());
		ui.cmbSettingsProfile->setCurrentIndex(0);

		connect(ui.cmbSettingsProfile, SIGNAL(currentIndexChanged(int)), SLOT(onMethodSettingsChanged(int)));
		connect(FDataManager->instance(),SIGNAL(settingsProfileInserted(const QUuid &, const QString &)),
		        SLOT(onSettingsProfileInserted(const QUuid &, const QString &)));
		connect(FDataManager->instance(),SIGNAL(settingsProfileRemoved(const QUuid &)), SLOT(onSettingsProfileRemoved(const QUuid &)));
	}

	connect(FFileStream->instance(),SIGNAL(stateChanged()),SLOT(onStreamStateChanged()));
	connect(FFileStream->instance(),SIGNAL(speedChanged()),SLOT(onStreamSpeedChanged()));
	connect(FFileStream->instance(),SIGNAL(propertiesChanged()),SLOT(onStreamPropertiesChanged()));
	connect(FFileStream->instance(),SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));

	connect(ui.tlbFile,SIGNAL(clicked(bool)),SLOT(onFileButtonClicked(bool)));
	connect(ui.bbxButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	onStreamPropertiesChanged();
	onStreamStateChanged();
	onStreamSpeedChanged();
}

StreamDialog::~StreamDialog()
{
	if (FFileStream)
	{
		if (FFileStream->streamState()==IFileStream::Finished || FFileStream->streamState()==IFileStream::Aborted)
			FFileStream->instance()->deleteLater();
		else if (FFileStream->streamKind()==IFileStream::SendFile && FFileStream->streamState()==IFileStream::Creating)
			FFileStream->instance()->deleteLater();
	}
	emit dialogDestroyed();
}

IFileStream *StreamDialog::stream() const
{
	return FFileStream;
}

void StreamDialog::setContactName(const QString &AName)
{
	ui.lblContact->setText(AName);
}

QList<QString> StreamDialog::selectedMethods() const
{
	QList<QString> methods;
	foreach (QCheckBox *button, FMethodButtons.keys())
		if (button->isChecked())
			methods.append(FMethodButtons.value(button));
	return methods;
}

void StreamDialog::setSelectableMethods(const QList<QString> &AMethods)
{
	qDeleteAll(FMethodButtons.keys());
	FMethodButtons.clear();
	foreach(QString methodNS, AMethods)
	{
		IDataStreamMethod *stremMethod = FDataManager->method(methodNS);
		if (stremMethod)
		{
			QCheckBox *button = new QCheckBox(stremMethod->methodName(),ui.grbMethods);
			button->setToolTip(stremMethod->methodDescription());
			button->setAutoExclusive(FFileStream->streamKind() == IFileStream::ReceiveFile);
			button->setChecked(FFileStream->streamKind()==IFileStream::SendFile || Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString()==methodNS);
			ui.wdtMethods->layout()->addWidget(button);
			FMethodButtons.insert(button,methodNS);
		}
	}
}

bool StreamDialog::acceptFileName(const QString &AFile)
{
	QFileInfo fileInfo(AFile);
	if (fileInfo.exists() && FFileStream->streamKind()==IFileStream::ReceiveFile)
	{
		if (FFileStream->isRangeSupported() && fileInfo.size()<FFileStream->fileSize())
		{
			QMessageBox::StandardButton button = QMessageBox::question(this,tr("Continue file transfer"),
			                                     tr("A file with this name, but a smaller size already exists.")+"<br>"+
			                                     tr("If you want to download the rest of file press 'Yes'")+"<br>"+
			                                     tr("If you want to start download from the beginning press 'Retry'")+"<br>"+
			                                     tr("If you want to change file name press 'Cancel'"),
			                                     QMessageBox::Yes|QMessageBox::Retry|QMessageBox::Cancel);
			if (button == QMessageBox::Yes)
			{
				FFileStream->setRangeOffset(fileInfo.size());
			}
			else if (button == QMessageBox::Retry)
			{
				if (!QFile::remove(fileInfo.absoluteFilePath()))
				{
					QMessageBox::warning(this,tr("Warning"),tr("Can not delete existing file"));
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			QMessageBox::StandardButton button = QMessageBox::question(this,tr("Remove file"),
			                                     tr("A file with this name already exists. Do you want to remove existing file?"),
			                                     QMessageBox::Yes|QMessageBox::Cancel);
			if (button == QMessageBox::Yes)
			{
				if (!QFile::remove(AFile))
				{
					QMessageBox::warning(this,tr("Warning"),tr("Can not delete existing file"));
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	else if (!fileInfo.exists() && FFileStream->streamKind()==IFileStream::SendFile)
	{
		QMessageBox::warning(this,tr("Warning"),tr("Selected file does not exists"));
		return false;
	}
	return !AFile.isEmpty();
}

QString StreamDialog::sizeName(qint64 ABytes) const
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

qint64 StreamDialog::minPosition() const
{
	return FFileStream->rangeOffset();
}

qint64 StreamDialog::maxPosition() const
{
	return FFileStream->rangeLength()>0 ? FFileStream->rangeOffset()+FFileStream->rangeLength() : FFileStream->fileSize();
}

qint64 StreamDialog::curPosition() const
{
	return minPosition() + FFileStream->progress();
}

int StreamDialog::curPercentPosition() const
{
	qint64 maxPos = maxPosition();
	return maxPos>0 ? curPosition()*100/maxPos : 0;
}

void StreamDialog::onStreamStateChanged()
{
	switch (FFileStream->streamState())
	{
	case IFileStream::Creating:
		ui.tlbFile->setEnabled(true);
		ui.lneFile->setReadOnly(FFileStream->streamKind()==IFileStream::SendFile);
		ui.pteDescription->setReadOnly(FFileStream->streamKind()!=IFileStream::SendFile);
		ui.grbMethods->setVisible(true);
		if (FFileStream->streamKind()==IFileStream::SendFile)
			ui.bbxButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
		else
			ui.bbxButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Abort);
		break;
	case IFileStream::Negotiating:
	case IFileStream::Connecting:
	case IFileStream::Transfering:
		ui.tlbFile->setEnabled(false);
		ui.lneFile->setReadOnly(true);
		ui.pteDescription->setReadOnly(true);
		ui.grbMethods->setVisible(false);
		ui.bbxButtons->setStandardButtons(QDialogButtonBox::Abort|QDialogButtonBox::Close);
		break;
	case IFileStream::Disconnecting:
	case IFileStream::Finished:
	case IFileStream::Aborted:
		ui.tlbFile->setEnabled(false);
		ui.lneFile->setReadOnly(true);
		ui.pteDescription->setReadOnly(true);
		ui.grbMethods->setVisible(false);
		if (FFileStream->streamKind()==IFileStream::SendFile && FFileStream->streamState()==IFileStream::Aborted)
			ui.bbxButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
		else if (FFileStream->streamKind()==IFileStream::ReceiveFile && FFileStream->streamState()==IFileStream::Finished)
			ui.bbxButtons->setStandardButtons(QDialogButtonBox::Open|QDialogButtonBox::Close);
		else
			ui.bbxButtons->setStandardButtons(QDialogButtonBox::Close);
		break;
	}
	ui.lblStatus->setText(FFileStream->stateString());
	resize(width(),minimumSizeHint().height());
}

void StreamDialog::onStreamSpeedChanged()
{
	if (FFileStream->streamState() == IFileStream::Transfering)
	{
		ui.pgbPrgress->setValue(curPercentPosition());
		ui.lblProgress->setText(tr("Transferred %1 of %2.").arg(sizeName(curPosition())).arg(sizeName(maxPosition()))
		                        + " " + tr("Speed %1.").arg(sizeName(FFileStream->speed())+tr("/sec")));
	}
	else if (FFileStream->fileSize() > 0)
	{
		ui.pgbPrgress->setValue(curPercentPosition());
		ui.lblProgress->setText(tr("Transferred %1 of %2.").arg(sizeName(curPosition())).arg(sizeName(maxPosition())));
	}
	else
	{
		ui.pgbPrgress->setValue(0);
		ui.lblProgress->setText(QString::null);
	}
}

void StreamDialog::onStreamPropertiesChanged()
{
	ui.lneFile->setText(FFileStream->fileName());
	ui.pteDescription->setPlainText(FFileStream->fileDescription());
	onStreamSpeedChanged();
}

void StreamDialog::onStreamDestroyed()
{
	FFileStream = NULL;
	close();
}

void StreamDialog::onFileButtonClicked(bool)
{
	if (FFileStream->streamState() == IFileStream::Creating)
	{
		static QString lastSelectedPath = QDir::homePath();
		QString file = QDir(lastSelectedPath).absoluteFilePath(FFileStream->fileName());

		if (FFileStream->streamKind() == IFileStream::ReceiveFile)
			file = QFileDialog::getSaveFileName(this,tr("Select file for receive"),file,QString::null,NULL,QFileDialog::DontConfirmOverwrite);
		else
			file = QFileDialog::getOpenFileName(this,tr("Select file to send"),file);

		if (!file.isEmpty())
		{
			lastSelectedPath = QFileInfo(file).absolutePath();
			FFileStream->setFileName(file);
		}
	}
}

void StreamDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	if (ui.bbxButtons->standardButton(AButton) ==  QDialogButtonBox::Ok)
	{
		QList<QString> methods = selectedMethods();
		if (!methods.isEmpty())
		{
			if (acceptFileName(ui.lneFile->text()))
			{
				if (FFileStream->streamKind() == IFileStream::SendFile)
				{
					FFileStream->setFileName(ui.lneFile->text());
					FFileStream->setFileDescription(ui.pteDescription->toPlainText());
					if (!FFileStream->initStream(methods))
						QMessageBox::warning(this,tr("Warning"),tr("Unable to send request for file transfer, check settings and try again!"));
				}
				else
				{
					IDataStreamMethod *streamMethod = FDataManager->method(methods.first());
					if (streamMethod)
					{
						QString file = ui.lneFile->text();
						FFileStream->setFileName(ui.lneFile->text());
						FFileStream->setFileDescription(ui.pteDescription->toPlainText());
						if (!FFileStream->startStream(methods.first()))
							QMessageBox::warning(this,tr("Warning"),tr("Unable to start the file transfer, check settings and try again!"));
					}
					else
						QMessageBox::warning(this,tr("Warning"),tr("Selected data stream is not available"));
				}
			}
		}
		else
			QMessageBox::warning(this,tr("Warning"),tr("Please select at least one data stream"));
	}
	else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Abort)
	{
		if (QMessageBox::question(this,tr("Cancel file transfer"),tr("Are you sure you want to cancel a file transfer?"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
		{
			FFileStream->abortStream(XmppError(IERR_FILETRANSFER_TRANSFER_TERMINATED));
		}
	}
	else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Retry)
	{
		FFileTransfer->sendFile(FFileStream->streamJid(), FFileStream->contactJid(), FFileStream->fileName(), FFileStream->fileDescription());
		close();
	}
	else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Open)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(FFileStream->fileName()).absolutePath()));
	}
	else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Close)
	{
		close();
	}
	else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Cancel)
	{
		close();
	}
}

void StreamDialog::onMethodSettingsChanged(int AIndex)
{
	FFileStream->setSettingsProfile(ui.cmbSettingsProfile->itemData(AIndex).toString());
}

void StreamDialog::onSettingsProfileInserted(const QUuid &AProfileId, const QString &AName)
{
	int index = ui.cmbSettingsProfile->findData(AProfileId.toString());
	if (index >= 0)
		ui.cmbSettingsProfile->setItemText(index, AName);
	else
		ui.cmbSettingsProfile->addItem(AName, AProfileId.toString());
}

void StreamDialog::onSettingsProfileRemoved(const QUuid &AProfileId)
{
	ui.cmbSettingsProfile->removeItem(ui.cmbSettingsProfile->findData(AProfileId.toString()));
}
