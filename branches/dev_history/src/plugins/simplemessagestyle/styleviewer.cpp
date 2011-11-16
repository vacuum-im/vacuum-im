#include "styleviewer.h"

#include <QTextBlock>
#include <QMouseEvent>
#include <QTextDocumentFragment>

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

void StyleViewer::mouseReleaseEvent(QMouseEvent *AEvent)
{
   if (AEvent->button() == Qt::RightButton)
   {
      QTextCursor cursor = cursorForPosition(AEvent->pos());
      if (textCursor().selection().isEmpty() || textCursor().selectionStart()>cursor.position() || textCursor().selectionEnd()<cursor.position())
      {
         if (!anchorAt(AEvent->pos()).isEmpty())
         {
            QTextBlock block = cursor.block();
            for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
            {
               if (it.fragment().contains(cursor.position()))
               {
                  cursor.setPosition(it.fragment().position());
                  cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,it.fragment().length());
                  break;
               }
            }
         }
         else
         {
            cursor.select(QTextCursor::WordUnderCursor);
         }
         setTextCursor(cursor);
      }
   }
   QTextBrowser::mouseReleaseEvent(AEvent);
}
