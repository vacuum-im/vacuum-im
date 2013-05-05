#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QImage>
#include "utilsexport.h"

class UTILS_EXPORT ImageManager
{
public:
	static QImage grayscaled(const QImage &AImage);
	static QImage squared(const QImage &AImage, int ASize);
	static QImage roundSquared(const QImage &AImage, int ASize, int ARadius);
	static QImage addShadow(const QImage &AImage, const QColor &AColor, const QPoint &AOffset);
	static QImage colorized(const QImage &AImage, const QColor &AColor);
	static QImage opacitized(const QImage &AImage, double AOpacity = 0.5);
	static QImage addSpace(const QImage &AImage, int ALeft, int ATop, int ARight, int ABottom);
	static QImage rotatedImage(const QImage &AImage, qreal AAngle);
private:
	ImageManager();
};

#endif // IMAGEMANAGER_H
