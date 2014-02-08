#include "imagemanager.h"

#include <QBitmap>
#include <QPainter>

ImageManager::ImageManager()
{

}

QImage ImageManager::grayscaled(const QImage &AImage)
{
	QImage img = AImage;
	if (!AImage.isNull())
	{
		int pixels = img.width() * img.height();
		if (pixels*(int)sizeof(QRgb) <= img.byteCount())
		{
			QRgb *data = (QRgb *)img.bits();
			for (int i = 0; i < pixels; i++)
			{
				int val = qGray(data[i]);
				data[i] = qRgba(val, val, val, qAlpha(data[i]));
			}
		}
	}
	return img;
}

QImage ImageManager::squared(const QImage &AImage, int ASize)
{
	if (!AImage.isNull() && ((AImage.width()!=ASize) || (AImage.height()!=ASize)))
	{
		QImage squaredImage(ASize, ASize, QImage::Format_ARGB32);
		squaredImage.fill(QColor(0, 0, 0, 0).rgba());
		int w = AImage.width(), h = AImage.height();
		QPainter p(&squaredImage);
		QPoint offset(0,0);
		QImage copy = (w < h) ? ((h == ASize) ? AImage : AImage.scaledToHeight(ASize, Qt::SmoothTransformation)) : ((w == ASize) ? AImage : AImage.scaledToWidth(ASize, Qt::SmoothTransformation));
		w = copy.width();
		h = copy.height();
		if (w > h)
			offset.setY((ASize - h) / 2);
		else if (h > w)
			offset.setX((ASize - w) / 2);
		p.drawImage(offset, copy);
		p.end();
		return squaredImage;
	}
	return AImage;
}

QImage ImageManager::roundSquared(const QImage &AImage, int ASize, int ARadius)
{
	if (!AImage.isNull())
	{
		QImage shapeImg(QSize(ASize, ASize), QImage::Format_ARGB32_Premultiplied);
		shapeImg.fill(QColor(0, 0, 0, 0).rgba());

		QPainter sp(&shapeImg);
		sp.setRenderHint(QPainter::Antialiasing);
		sp.fillRect(0, 0, ASize, ASize, Qt::transparent);
		sp.setPen(QPen(Qt::color1));
		sp.setBrush(QBrush(Qt::color1));
		sp.drawRoundedRect(QRect(0, 0, ASize, ASize), ARadius + 1, ARadius + 1);
		sp.end();
		
		QImage roundSquaredImage(ASize, ASize, QImage::Format_ARGB32_Premultiplied);
		roundSquaredImage.fill(QColor(0, 0, 0, 0).rgba());
		
		QPainter p(&roundSquaredImage);
		p.fillRect(0, 0, ASize, ASize, Qt::transparent);
		p.drawImage(0, 0, shapeImg);
		p.setCompositionMode(QPainter::CompositionMode_SourceIn);
		p.drawImage(0, 0, squared(AImage, ASize));
		p.end();

		return roundSquaredImage;
	}
	return AImage;
}

QImage ImageManager::addShadow(const QImage &AImage, const QColor &AColor, const QPoint &AOffset)
{
	if (!AImage.isNull())
	{
		QImage shadowed(AImage.size(), AImage.format());
		shadowed.fill(QColor(0, 0, 0, 0).rgba());

		QImage tmp(AImage.size(), QImage::Format_ARGB32_Premultiplied);
		tmp.fill(0);

		QPainter tmpPainter(&tmp);
		tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
		tmpPainter.drawPixmap(AOffset, QPixmap::fromImage(AImage));
		tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tmpPainter.fillRect(tmp.rect(), AColor);
		tmpPainter.end();

		QPainter p(&shadowed);
		p.drawImage(0, 0, tmp);
		p.drawImage(0, 0, AImage);
		p.end();

		return shadowed;
	}
	return AImage;
}

QImage ImageManager::colorized(const QImage &AImage, const QColor &AColor)
{
	if (!AImage.isNull())
	{
		QImage resultImage(AImage.size(), QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&resultImage);
		painter.drawImage(0, 0, grayscaled(AImage));
		painter.setCompositionMode(QPainter::CompositionMode_Screen);
		painter.fillRect(resultImage.rect(), AColor);
		painter.end();
		resultImage.setAlphaChannel(AImage.alphaChannel());
		return resultImage;
	}
	return AImage;
}

QImage ImageManager::opacitized(const QImage &AImage, double AOpacity)
{
	if (!AImage.isNull())
	{
		QImage resultImage(AImage.size(), QImage::Format_ARGB32);
		resultImage.fill(QColor::fromRgb(0, 0, 0, 0).rgba());
		QPainter painter(&resultImage);
		painter.setOpacity(AOpacity);
		painter.drawImage(0, 0, AImage);
		painter.end();
		resultImage.setAlphaChannel(AImage.alphaChannel());
		return resultImage;
	}
	return AImage;
}

QImage ImageManager::addSpace(const QImage &AImage, int ALeft, int ATop, int ARight, int ABottom)
{
	if (!AImage.isNull())
	{
		QImage resultImage(AImage.size() + QSize(ALeft + ARight, ATop + ABottom), QImage::Format_ARGB32);
		resultImage.fill(QColor::fromRgb(0, 0, 0, 0).rgba());
		QPainter painter(&resultImage);
		painter.drawImage(ALeft, ATop, AImage);
		painter.end();
		resultImage.setAlphaChannel(AImage.alphaChannel());
		return resultImage;
	}
	return AImage;
}

QImage ImageManager::rotatedImage(const QImage &AImage, qreal AAngle)
{
	if (!AImage.isNull())
	{
		QImage rotated(AImage.size(), QImage::Format_ARGB32_Premultiplied);
		rotated.fill(Qt::transparent);
		QPainter p(&rotated);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		qreal dx = AImage.size().width() / 2.0, dy = AImage.size().height() / 2.0;
		p.translate(dx, dy);
		p.rotate(AAngle);
		p.translate(-dx, -dy);
		p.drawImage(0, 0, AImage);
		p.end();
		return rotated;
	}
	return AImage;
}
