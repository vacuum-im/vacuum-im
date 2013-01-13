#ifndef PRIXMAPFRAME_H
#define PRIXMAPFRAME_H

#include <QTimer>
#include <QFrame>
#include <QBuffer>
#include <QImageReader>

class PrixmapFrame : 
	public QFrame
{
	Q_OBJECT;
public:
	PrixmapFrame(QWidget *AParent);
	~PrixmapFrame();
	QByteArray imageData() const;
	void setImageData(const QByteArray &AData);
public:
	virtual QSize sizeHint() const;
protected:
	void resetReader();
	void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onUpdateFrameTimeout();
private:
	QTimer FTimer;
	QImage FCurFrame;
	QBuffer FImageBuffer;
	QByteArray FImageData;
	QImageReader FImageReader;
};

#endif // PRIXMAPFRAME_H
