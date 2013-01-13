#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QTextBrowser>

class StyleViewer : 
  public QTextBrowser
{
  Q_OBJECT;
public:
  StyleViewer(QWidget *AParent);
  ~StyleViewer();
};

#endif // STYLEVIEWER_H
