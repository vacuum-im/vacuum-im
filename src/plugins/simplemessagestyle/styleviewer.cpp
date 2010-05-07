#include "styleviewer.h"

StyleViewer::StyleViewer(QWidget *AParent) : QTextBrowser(AParent)
{
	setOpenLinks(false);
	setAcceptDrops(false);
	setOpenExternalLinks(false);
	setFrameShape(QFrame::NoFrame);
}

StyleViewer::~StyleViewer()
{

}
