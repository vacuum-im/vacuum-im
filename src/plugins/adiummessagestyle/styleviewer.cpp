#include "styleviewer.h"

#include "webpage.h"

StyleViewer::StyleViewer(QWidget *AParent) : QWebEngineView(AParent)
{
	WebPage *webPage = new WebPage(this);
	connect(webPage, SIGNAL(linkClicked(const QUrl &)), SIGNAL(linkClicked(const QUrl &)));

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
