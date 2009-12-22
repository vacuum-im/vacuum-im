#include "styleviewer.h"

#include <QEvent>
#include <QMouseEvent>

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

bool StyleViewer::event(QEvent *AEvent)
{
  if (AEvent->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(AEvent);
    if (mouseEvent && mouseEvent->buttons()==Qt::MidButton)
      return false;
  }
  return QTextBrowser::event(AEvent);
}
