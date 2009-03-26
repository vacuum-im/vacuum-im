#include "texteditor.h"

TextEditor::TextEditor(QWidget *AParent) : QTextEdit(AParent)
{

}

TextEditor::~TextEditor()
{

}

QVariant TextEditor::loadResource(int AType, const QUrl &AName)
{
  QVariant value;
  emit loadCustomResource(AType,AName,value);
  return value.isValid() ? value : QTextEdit::loadResource(AType,AName);
}
