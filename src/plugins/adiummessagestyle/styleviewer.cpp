#include "styleviewer.h"

#include <QCursor>
#include <QMouseEvent>
#include <QApplication>
#include <QWebEngineProfile>
#include "webpage.h"

StyleViewer::StyleViewer(QWidget *AParent) : QWebEngineView(AParent)
{
	QWebEngineProfile *webProfile = new QWebEngineProfile(this);

	FHitTimer.setInterval(200);
	FHitTimer.setSingleShot(false);
	connect(&FHitTimer, SIGNAL(timeout()), SLOT(onHitTimerTimeout()));

	WebPage *webPage = new WebPage(webProfile, this);
	connect(webPage, SIGNAL(linkClicked(const QUrl &)), SIGNAL(linkClicked(const QUrl &)));
	connect(webPage, SIGNAL(webHitTestResult(const QString &, const WebHitTestResult &)), SLOT(onWebPageHitTestResult(const QString &, const WebHitTestResult &)));

	setPage(webPage);
	setAcceptDrops(false);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

QSize StyleViewer::sizeHint() const
{
	return QSize(256,192);
}

QSize StyleViewer::minimumSizeHint() const
{
	return QSize(70,50);
}

WebHitTestResult StyleViewer::hitTestContent(const QPoint &APosition) const
{
	if ((APosition - FHitPoint).manhattanLength() <= QApplication::startDragDistance())
		return FHitResult;
	return WebHitTestResult();
}

void StyleViewer::enterEvent(QEvent *AEvent)
{
	Q_UNUSED(AEvent);
	FHitTimer.start();
}

void StyleViewer::leaveEvent(QEvent *AEvent)
{
	Q_UNUSED(AEvent);
	FHitTimer.stop();
}

void StyleViewer::onHitTimerTimeout()
{
	QPoint currPos = mapFromGlobal(QCursor::pos());
	if (FHitRequestId.isEmpty() && (FHitPoint - currPos).manhattanLength()>QApplication::startDragDistance())
	{
		FHitPoint = currPos;
		WebPage *webpage = qobject_cast<WebPage *>(page());
		FHitRequestId = webpage!=NULL ? webpage->requestWebHitTest(FHitPoint) : QString::null;
	}
}

void StyleViewer::onWebPageHitTestResult(const QString &AId, const WebHitTestResult &AResult)
{
	if (FHitRequestId == AId)
	{
		FHitResult = AResult;
		FHitRequestId.clear();
	}
}
