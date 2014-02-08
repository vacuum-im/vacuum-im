#include "styleviewer.h"

#include <QTextBlock>
#include <QMouseEvent>
#include <QTextDocumentFragment>

StyleViewer::StyleViewer(QWidget *AParent) : AnimatedTextBrowser(AParent)
{
	setOpenLinks(false);
	setAcceptDrops(false);
	setUndoRedoEnabled(false);
	setOpenExternalLinks(false);
	setFrameShape(QFrame::NoFrame);
	setContextMenuPolicy(Qt::CustomContextMenu);
}

StyleViewer::~StyleViewer()
{

}

QSize StyleViewer::sizeHint() const
{
	return QSize(256,192);
}

QSize StyleViewer::minimumSizeHint() const
{
	return QSize(70,50);
}
