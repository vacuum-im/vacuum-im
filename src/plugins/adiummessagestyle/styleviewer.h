#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QWebEngineView>

class StyleViewer :
	public QWebEngineView
{
	Q_OBJECT;
public:
	StyleViewer(QWidget *AParent);
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
signals:
	void linkClicked(const QUrl &AUrl);
};

#endif // STYLEVIEWER_H
