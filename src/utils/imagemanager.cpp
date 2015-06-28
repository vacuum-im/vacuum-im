#include "imagemanager.h"

#include <QBitmap>
#include <QPainter>

ImageManager::ImageManager()
{

}

QImage ImageManager::grayscaled(const QImage &AImage)
{
	if (!AImage.isNull() && !AImage.isGrayscale())
	{
		QImage result = AImage;
		int pixels = result.width() * result.height();
		if (pixels*(int)sizeof(QRgb) <= result.byteCount())
		{
			QRgb *data = (QRgb *)result.bits();
			for (int i = 0; i<pixels; i++)
			{
				int val = qGray(data[i]);
				data[i] = qRgba(val, val, val, qAlpha(data[i]));
			}
			return result;
		}
	}
	return AImage;
}

QImage ImageManager::squared(const QImage &AImage, int ASize)
{
	if (!AImage.isNull() && ((AImage.width()!=ASize) || (AImage.height()!=ASize)))
	{
		QImage result(ASize, ASize, QImage::Format_ARGB32);
		result.fill(QColor(0, 0, 0, 0).rgba());

		int w = AImage.width();
		int h = AImage.height();
		QImage scaled = AImage;
		if (w<h && h!=ASize)
			scaled = AImage.scaledToHeight(ASize, Qt::SmoothTransformation);
		else if (w>=h && w!=ASize)
			scaled = AImage.scaledToWidth(ASize, Qt::SmoothTransformation);

		w = scaled.width();
		h = scaled.height();
		QPoint offset(0,0);
		if (w > h)
			offset.setY((ASize - h) / 2);
		else if (h > w)
			offset.setX((ASize - w) / 2);

		QPainter p(&result);
		p.drawImage(offset, scaled);
		p.end();

		return result;
	}
	return AImage;
}

QImage ImageManager::roundSquared(const QImage &AImage, int ASize, int ARadius)
{
	if (!AImage.isNull())
	{
		QImage shapeImage(QSize(ASize, ASize), QImage::Format_ARGB32_Premultiplied);
		shapeImage.fill(QColor(0, 0, 0, 0).rgba());

		QPainter sp(&shapeImage);
		sp.setRenderHint(QPainter::Antialiasing);
		sp.fillRect(0, 0, ASize, ASize, Qt::transparent);
		sp.setPen(QPen(Qt::color1));
		sp.setBrush(QBrush(Qt::color1));
		sp.drawRoundedRect(QRect(0, 0, ASize, ASize), ARadius + 1, ARadius + 1);
		sp.end();
	
		QImage result(ASize, ASize, QImage::Format_ARGB32_Premultiplied);
		result.fill(QColor(0, 0, 0, 0).rgba());

		QPainter p(&result);
		p.fillRect(0, 0, ASize, ASize, Qt::transparent);
		p.drawImage(0, 0, shapeImage);
		p.setCompositionMode(QPainter::CompositionMode_SourceIn);
		p.drawImage(0, 0, squared(AImage, ASize));
		p.end();

		return result;
	}
	return AImage;
}

QImage ImageManager::addShadow(const QImage &AImage, const QColor &AColor, const QPoint &AOffset)
{
	if (!AImage.isNull())
	{
		QImage result(AImage.size(), AImage.format());
		result.fill(QColor(0, 0, 0, 0).rgba());

		QImage shadow(AImage.size(), QImage::Format_ARGB32_Premultiplied);
		shadow.fill(0);

		QPainter sp(&shadow);
		sp.setCompositionMode(QPainter::CompositionMode_Source);
		sp.drawPixmap(AOffset, QPixmap::fromImage(AImage));
		sp.setCompositionMode(QPainter::CompositionMode_SourceIn);
		sp.fillRect(shadow.rect(), AColor);
		sp.end();

		QPainter p(&result);
		p.drawImage(0, 0, shadow);
		p.drawImage(0, 0, AImage);
		p.end();

		return result;
	}
	return AImage;
}

QImage ImageManager::colorized(const QImage &AImage, const QColor &AColor)
{
	if (!AImage.isNull())
	{
		QImage result(AImage.size(), QImage::Format_ARGB32_Premultiplied);

		QPainter painter(&result);
		painter.drawImage(0, 0, grayscaled(AImage));
		painter.setCompositionMode(QPainter::CompositionMode_Screen);
		painter.fillRect(result.rect(), AColor);
		painter.end();

		result.setAlphaChannel(AImage.alphaChannel());
		return result;
	}
	return AImage;
}

QImage ImageManager::opacitized(const QImage &AImage, double AOpacity)
{
	if (!AImage.isNull())
	{
		QImage result(AImage.size(), QImage::Format_ARGB32);
		result.fill(QColor::fromRgb(0, 0, 0, 0).rgba());

		QPainter painter(&result);
		painter.setOpacity(AOpacity);
		painter.drawImage(0, 0, AImage);
		painter.end();

		result.setAlphaChannel(AImage.alphaChannel());
		return result;
	}
	return AImage;
}

QImage ImageManager::addSpace(const QImage &AImage, int ALeft, int ATop, int ARight, int ABottom)
{
	if (!AImage.isNull())
	{
		QImage result(AImage.size() + QSize(ALeft + ARight, ATop + ABottom), QImage::Format_ARGB32);
		result.fill(QColor::fromRgb(0, 0, 0, 0).rgba());

		QPainter painter(&result);
		painter.drawImage(ALeft, ATop, AImage);
		painter.end();

		result.setAlphaChannel(AImage.alphaChannel());
		return result;
	}
	return AImage;
}

QImage ImageManager::rotatedImage(const QImage &AImage, qreal AAngle)
{
	if (!AImage.isNull())
	{
		QImage result(AImage.size(), QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);

		qreal dx = AImage.size().width() / 2.0;
		qreal dy = AImage.size().height() / 2.0;

		QPainter p(&result);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.translate(dx, dy);
		p.rotate(AAngle);
		p.translate(-dx, -dy);
		p.drawImage(0, 0, AImage);
		p.end();
		
		return result;
	}
	return AImage;
}
