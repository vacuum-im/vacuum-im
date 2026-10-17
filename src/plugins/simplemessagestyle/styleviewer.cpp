#include "styleviewer.h"

#include <QDesktopServices>
#include <QMouseEvent>

StyleViewer::StyleViewer(QWidget *AParent) : AnimatedTextBrowser(AParent)
{
	setAnimated(true);
	setOpenLinks(false);
	setAcceptDrops(false);
	setUndoRedoEnabled(false);
	setOpenExternalLinks(false);
	setFrameShape(QFrame::NoFrame);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &StyleViewer::anchorClicked, this, &StyleViewer::onLinkActivated);
}

StyleViewer::~StyleViewer()
{

}

void StyleViewer::onLinkActivated(const QUrl &AUrl)
{
	QDesktopServices::openUrl(AUrl);
}

void StyleViewer::mousePressEvent(QMouseEvent *AEvent)
{
	QTextBrowser::mousePressEvent(AEvent);
}

QSize StyleViewer::sizeHint() const
{
	return QSize(256,192);
}

QSize StyleViewer::minimumSizeHint() const
{
	return QSize(70,50);
}
