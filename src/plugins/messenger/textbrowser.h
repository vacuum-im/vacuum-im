#ifndef TEXTBROWSER_H
#define TEXTBROWSER_H

#include <QTextBrowser>

class TextBrowser : 
  public QTextBrowser
{
  Q_OBJECT;
public:
  TextBrowser(QWidget *AParent = NULL);
  ~TextBrowser();
  virtual QVariant loadResource(int AType, const QUrl &AName);
signals:
  void loadCustomResource(int AType, const QUrl &AName, QVariant &AValue);
};

#endif // TEXTBROWSER_H
