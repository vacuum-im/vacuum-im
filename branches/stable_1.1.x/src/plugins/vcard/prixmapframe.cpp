#include "prixmapframe.h"

#include <QRect>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>

PrixmapFrame::PrixmapFrame(QWidget *AParent) : QFrame(AParent)
{

}

PrixmapFrame::~PrixmapFrame()
{

}

QPixmap PrixmapFrame::pixmap() const
{
	return FPixmap;
}

void PrixmapFrame::setPixmap(const QPixmap &APixmap)
{
	FPixmap = APixmap;
}

QSize PrixmapFrame::sizeHint() const
{
	return FPixmap.size() + QSize(frameWidth()*2,frameWidth()*2);
}

void PrixmapFrame::paintEvent(QPaintEvent *AEvent)
{
	QFrame::paintEvent(AEvent);

	QRect rect = AEvent->rect();
	rect.adjust(frameWidth(),frameWidth(),-frameWidth(),-frameWidth());

	QSize size = FPixmap.size();
	size.scale(rect.size(),Qt::KeepAspectRatio);

	QPainter painter(this);
	painter.drawPixmap(QStyle::alignedRect(layoutDirection(),Qt::AlignCenter,size,rect),FPixmap);
}
