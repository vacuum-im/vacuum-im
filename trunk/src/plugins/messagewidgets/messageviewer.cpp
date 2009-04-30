#include "messageviewer.h"

MessageViewer::MessageViewer(QWidget *AParent) : QWebView(AParent)
{
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

MessageViewer::~MessageViewer()
{

}

QSize MessageViewer::sizeHint() const
{
  return QSize(256,192);
}