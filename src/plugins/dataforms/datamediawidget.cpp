#include "datamediawidget.h"

#include <QMovie>
#include <QBuffer>
#include <QPixmap>
#include <QImageReader>
#include <QTextDocument>

DataMediaWidget::DataMediaWidget(IDataForms *ADataForms, const IDataMedia &AMedia, QWidget *AParent) : QLabel(AParent)
{
  FMedia = AMedia;
  FDataForms = ADataForms;

  setFrameShape(QLabel::Panel);
  setFrameShadow(QLabel::Sunken);
  
  connect(FDataForms->instance(),SIGNAL(urlLoaded(const QUrl &, const QByteArray &)),SLOT(onUrlLoaded(const QUrl &, const QByteArray &)));
  connect(FDataForms->instance(),SIGNAL(urlLoadFailed(const QUrl &, const QString &)),SLOT(onUrlLoadFailed(const QUrl &, const QString &)));

  uriIndex = 0;
  FLastError = tr("Unsupported media type");
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
  return FMedia.uris.value(uriIndex);
}

void DataMediaWidget::loadUri()
{
  if (uriIndex < FMedia.uris.count())
  {
    const IDataMediaURI &uri = FMedia.uris.at(uriIndex);
    if (FDataForms->isSupportedUri(uri))
    {
      setToolTip(uri.url.toString());
      setText(tr("Loading data..."));
      FDataForms->loadUrl(uri.url);
    }
    else
    {
      uriIndex++;
      loadUri();
    }
  }
  else
  {
    disconnect(FDataForms->instance());
    setText(Qt::escape(FLastError));
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
        delete movie;
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
      delete buffer;
  }
  return success;
}

void DataMediaWidget::onUrlLoaded(const QUrl &AUrl, const QByteArray &AData)
{
  if (uriIndex<FMedia.uris.count() && FMedia.uris.at(uriIndex).url == AUrl)
  {
    const IDataMediaURI &uri = FMedia.uris.at(uriIndex);
    if (!updateWidget(uri, AData))
    {
      uriIndex++;
      FLastError = tr("Unsupported data format");
      loadUri();
    }
  }
}

void DataMediaWidget::onUrlLoadFailed(const QUrl &AUrl, const QString &AError)
{
  if (uriIndex<FMedia.uris.count() && FMedia.uris.at(uriIndex).url == AUrl)
  {
    uriIndex++;
    FLastError = AError;
    loadUri();
  }
}

