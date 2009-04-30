#ifndef MESSAGEVIEWER_H
#define MESSAGEVIEWER_H

#include <QWebView>

class MessageViewer : 
  public QWebView
{
  Q_OBJECT;
public:
  MessageViewer(QWidget *AParent);
  ~MessageViewer();
public:
  virtual QSize sizeHint() const;
};

#endif // MESSAGEVIEWER_H
