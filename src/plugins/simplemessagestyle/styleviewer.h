#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QTextEdit>

class StyleViewer : 
  public QTextEdit
{
  Q_OBJECT;
public:
  StyleViewer(QWidget *AParent);
  ~StyleViewer();
};

#endif // STYLEVIEWER_H
