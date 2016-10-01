#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QTimer>
#include <QWebEngineView>
#include "webhittestresult.h"

class StyleViewer :
	public QWebEngineView
{
	Q_OBJECT;
public:
	StyleViewer(QWidget *AParent);
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	WebHitTestResult hitTestContent(const QPoint &APosition) const;
private:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
signals:
	void linkClicked(const QUrl &AUrl);
private slots:
	void onHitTimerTimeout();
	void onWebPageHitTestResult(const QString &AId, const WebHitTestResult &AResult);
private:
	QTimer FHitTimer;
	QPoint FHitPoint;
	QString FHitRequestId;
	WebHitTestResult FHitResult;
};

#endif // STYLEVIEWER_H
