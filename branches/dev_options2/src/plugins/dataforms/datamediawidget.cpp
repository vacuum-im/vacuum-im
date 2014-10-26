#include "datamediawidget.h"

#include <QMovie>
#include <QBuffer>
#include <QPixmap>
#include <QImageReader>
#include <QTextDocument>
#include <definitions/internalerrors.h>

DataMediaWidget::DataMediaWidget(IDataForms *ADataForms, const IDataMedia &AMedia, QWidget *AParent) : QLabel(AParent)
{
	FMedia = AMedia;
	FDataForms = ADataForms;

	setTextFormat(Qt::PlainText);
	setFrameShape(QLabel::Panel);
	setFrameShadow(QLabel::Sunken);

	connect(FDataForms->instance(),SIGNAL(urlLoaded(const QUrl &, const QByteArray &)),SLOT(onUrlLoaded(const QUrl &, const QByteArray &)));
	connect(FDataForms->instance(),SIGNAL(urlLoadFailed(const QUrl &, const XmppError &)),SLOT(onUrlLoadFailed(const QUrl &, const XmppError &)));

	FUriIndex = 0;
	FLastError = XmppError(IERR_DATAFORMS_MEDIA_INVALID_TYPE);

	loadUri();
}

DataMediaWidget::~DataMediaWidget()
{

}

IDataMedia DataMediaWidget::media() const
{
	return FMedia;
}

IDataMediaURI DataMediaWidget::mediaUri() const
{
	return FMedia.uris.value(FUriIndex);
}

void DataMediaWidget::loadUri()
{
	if (FUriIndex < FMedia.uris.count())
	{
		const IDataMediaURI &uri = FMedia.uris.at(FUriIndex);
		if (FDataForms->isSupportedUri(uri))
		{
			setToolTip(uri.url.toString());
			setText(tr("Loading data..."));
			FDataForms->loadUrl(uri.url);
		}
		else
		{
			FUriIndex++;
			loadUri();
		}
	}
	else
	{
		disconnect(FDataForms->instance());
		setText(FLastError.errorMessage());
		emit mediaError(FLastError);
	}
}

bool DataMediaWidget::updateWidget(const IDataMediaURI &AUri, const QByteArray &AData)
{
	bool success = false;
	if (AUri.type == MEDIAELEM_TYPE_IMAGE)
	{
		QBuffer *buffer = new QBuffer(this);
		buffer->setData(AData);
		buffer->open(QIODevice::ReadOnly);

		QImageReader reader(buffer);
		if (reader.supportsAnimation())
		{
			QMovie *movie = new QMovie(buffer,reader.format(),this);
			if (movie->isValid())
			{
				success = true;
				setMovie(movie);
				movie->start();
			}
			else
			{
				delete movie;
			}
		}
		else
		{
			QPixmap pixmap;
			pixmap.loadFromData(AData,reader.format());
			if (!pixmap.isNull())
			{
				success = true;
				setPixmap(pixmap);
			}
		}
		if (success)
		{
			setFrameShape(QLabel::NoFrame);
			setFrameShadow(QLabel::Plain);
			disconnect(FDataForms->instance());
			emit mediaShown();
		}
		else
		{
			delete buffer;
		}
	}
	return success;
}

void DataMediaWidget::onUrlLoaded(const QUrl &AUrl, const QByteArray &AData)
{
	if (FUriIndex<FMedia.uris.count() && FMedia.uris.at(FUriIndex).url == AUrl)
	{
		const IDataMediaURI &uri = FMedia.uris.at(FUriIndex);
		if (!updateWidget(uri, AData))
		{
			FUriIndex++;
			FLastError = XmppError(IERR_DATAFORMS_MEDIA_INVALID_FORMAT);
			loadUri();
		}
	}
}

void DataMediaWidget::onUrlLoadFailed(const QUrl &AUrl, const XmppError &AError)
{
	if (FUriIndex<FMedia.uris.count() && FMedia.uris.at(FUriIndex).url == AUrl)
	{
		FUriIndex++;
		FLastError = AError;
		loadUri();
	}
}
