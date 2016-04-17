#include "styleviewer.h"

#include <QShortcut>

#include "webpage.h"

StyleViewer::StyleViewer(QWidget *AParent) : QWebEngineView(AParent)
{
	WebPage *webPage = new WebPage(this);
	connect(webPage, SIGNAL(linkCliked(const QUrl &)), SIGNAL(linkClicked(const QUrl &)));

	QShortcut *shortcut = new QShortcut(QKeySequence::Copy, this, NULL, NULL, Qt::WidgetShortcut);
	connect(shortcut, SIGNAL(activated()), SLOT(onShortcutActivated()));

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

void StyleViewer::onShortcutActivated()
{
	triggerPageAction(QWebEnginePage::Copy);
}
