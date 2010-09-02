#ifndef PRIXMAPFRAME_H
#define PRIXMAPFRAME_H

#include <QFrame>

class PrixmapFrame : 
	public QFrame
{
	Q_OBJECT;
public:
	PrixmapFrame(QWidget *AParent);
	~PrixmapFrame();
	QPixmap pixmap() const;
	void setPixmap(const QPixmap &APixmap);
public:
	virtual QSize sizeHint() const;
protected:
	virtual void paintEvent(QPaintEvent *AEvent);
private:
	QPixmap FPixmap;
};

#endif // PRIXMAPFRAME_H
