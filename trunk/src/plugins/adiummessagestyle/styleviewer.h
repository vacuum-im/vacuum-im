#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QWebView>

class StyleViewer : 
  public QWebView
{
  Q_OBJECT;
public:
  StyleViewer(QWidget *AParent);
  ~StyleViewer();
public:
  virtual QSize sizeHint() const;
protected slots:
  void onShortcutActivated();
};

#endif // STYLEVIEWER_H
