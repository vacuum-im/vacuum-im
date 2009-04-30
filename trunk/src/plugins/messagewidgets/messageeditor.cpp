#include "messageeditor.h"

#include <QFrame>
#include <QAbstractTextDocumentLayout> 

MessageEditor::MessageEditor(QWidget *AParent) : QTextEdit(AParent)
{
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  connect(this,SIGNAL(textChanged()),SLOT(onTextChanged()));
}

MessageEditor::~MessageEditor()
{

}

QSize MessageEditor::sizeHint() const
{
  QSize sh = QTextEdit::sizeHint();
  sh.setHeight(qRound(document()->documentLayout()->documentSize().height()) + frameWidth()*2 + 1);
  return sh;
}

QSize MessageEditor::minimumSizeHint() const
{
  QSize sh = QTextEdit::minimumSizeHint();
  sh.setHeight(fontMetrics().height() + frameWidth()*2 + qRound(document()->documentMargin())*2);
  return sh;
}

void MessageEditor::onTextChanged()
{
  updateGeometry();
}
