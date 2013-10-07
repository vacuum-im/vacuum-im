#include "prixmapframe.h"

#include <QRect>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>

PrixmapFrame::PrixmapFrame(QWidget *AParent) : QFrame(AParent)
{
	FTimer.setSingleShot(true);
	connect(&FTimer,SIGNAL(timeout()),SLOT(onUpdateFrameTimeout()));
}

PrixmapFrame::~PrixmapFrame()
{
	FTimer.stop();
}

QByteArray PrixmapFrame::imageData() const
{
	return FImageData;
}

void PrixmapFrame::setImageData(const QByteArray &AData)
{
	FImageData = AData;
	resetReader();

	FCurFrame = FImageReader.read();
	
	if (!FCurFrame.isNull() && FImageReader.nextImageDelay()>0)
		FTimer.start(FImageReader.nextImageDelay());

	update();
}

QSize PrixmapFrame::sizeHint() const
{
	return FCurFrame.size() + QSize(frameWidth()*2,frameWidth()*2);
}

void PrixmapFrame::resetReader()
{
	FImageReader.setDevice(NULL);
	FImageBuffer.close();
	FTimer.stop();

	FImageBuffer.setData(FImageData);
	FImageBuffer.open(QBuffer::ReadOnly);

	FImageReader.setDevice(&FImageBuffer);
}

void PrixmapFrame::paintEvent(QPaintEvent *AEvent)
{
	QFrame::paintEvent(AEvent);

	QRect rect = AEvent->rect();
	rect.adjust(frameWidth(),frameWidth(),-frameWidth(),-frameWidth());

	QSize size = FCurFrame.size();
	if (size.width()>rect.size().width() || size.height()>rect.size().height())
		size.scale(rect.size(),Qt::KeepAspectRatio);

	QPainter painter(this);
	painter.drawImage(QStyle::alignedRect(layoutDirection(),Qt::AlignCenter,size,rect),FCurFrame);
}

void PrixmapFrame::onUpdateFrameTimeout()
{
	FCurFrame = FImageReader.read();
	if (FCurFrame.isNull())
	{
		resetReader();
		FCurFrame = FImageReader.read();
	}
	FTimer.start(FImageReader.nextImageDelay());
	update();
}
