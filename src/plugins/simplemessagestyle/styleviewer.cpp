#include "styleviewer.h"

StyleViewer::StyleViewer(QWidget *AParent) : QTextBrowser(AParent)
{
  setFrameShape(QFrame::NoFrame);
  setOpenExternalLinks(false);
  setOpenLinks(false);
}

StyleViewer::~StyleViewer()
{

}
