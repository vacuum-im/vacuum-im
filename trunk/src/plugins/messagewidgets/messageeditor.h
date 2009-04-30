#ifndef MESSAGEEDITOR_H
#define MESSAGEEDITOR_H

#include <QTextEdit>

class MessageEditor : 
  public QTextEdit
{
  Q_OBJECT;
public:
  MessageEditor(QWidget *AParent);
  ~MessageEditor();
public:
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;
protected slots:
  void onTextChanged();
};

#endif // MESSAGEEDITOR_H
