#include "styleviewer.h"

#include <QTextBlock>
#include <QMouseEvent>
#include <QTextDocumentFragment>

StyleViewer::StyleViewer(QWidget *AParent) : AnimatedTextBrowser(AParent)
{
	setOpenLinks(false);
	setAcceptDrops(false);
	setOpenExternalLinks(false);
	setFrameShape(QFrame::NoFrame);
	setContextMenuPolicy(Qt::CustomContextMenu);
}

StyleViewer::~StyleViewer()
{

}
