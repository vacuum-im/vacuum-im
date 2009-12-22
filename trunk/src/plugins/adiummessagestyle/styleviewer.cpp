#include "styleviewer.h"

#include <QEvent>
#include <QShortcut>
#include <QMouseEvent>

StyleViewer::StyleViewer(QWidget *AParent) : QWebView(AParent)
{
  setAcceptDrops(false);
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  QShortcut *shortcut = new QShortcut(QKeySequence::Copy, this);
  connect(shortcut, SIGNAL(activated()), SLOT(onShortcutActivated()));
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
  return QWebView::event(AEvent);
}

QSize StyleViewer::sizeHint() const
{
  return QSize(256,192);
}

void StyleViewer::onShortcutActivated()
{
  triggerPageAction(QWebPage::Copy);
}
