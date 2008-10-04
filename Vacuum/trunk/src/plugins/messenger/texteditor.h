#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QTextEdit>

class TextEditor : 
  public QTextEdit
{
  Q_OBJECT;
public:
  TextEditor(QWidget *AParent = NULL);
  ~TextEditor();
  virtual QVariant loadResource(int AType, const QUrl &AName);
signals:
  void loadCustomResource(int AType, const QUrl &AName, QVariant &AValue);
};

#endif // TEXTEDITOR_H
