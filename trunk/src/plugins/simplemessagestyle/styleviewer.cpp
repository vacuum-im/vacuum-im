#include "styleviewer.h"

StyleViewer::StyleViewer(QWidget *AParent) : QTextEdit(AParent)
{
  setReadOnly(true);
  setLineWrapMode(QTextEdit::WidgetWidth);
  setWordWrapMode(QTextOption::WordWrap);
}

StyleViewer::~StyleViewer()
{

}
