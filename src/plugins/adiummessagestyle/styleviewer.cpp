#include "styleviewer.h"

#include <QShortcut>

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

QSize StyleViewer::sizeHint() const
{
  return QSize(256,192);
}

void StyleViewer::onShortcutActivated()
{
  triggerPageAction(QWebPage::Copy);
}
