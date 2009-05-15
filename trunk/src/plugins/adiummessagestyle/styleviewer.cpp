#include "styleviewer.h"

StyleViewer::StyleViewer(QWidget *AParent) : QWebView(AParent)
{
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

StyleViewer::~StyleViewer()
{

}

QSize StyleViewer::sizeHint() const
{
  return QSize(256,192);
}