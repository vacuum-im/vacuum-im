#include "textbrowser.h"

TextBrowser::TextBrowser(QWidget *AParent) : QTextBrowser(AParent)
{

}

TextBrowser::~TextBrowser()
{

}

QVariant TextBrowser::loadResource(int AType, const QUrl &AName)
{
  QVariant value;
  emit loadCustomResource(AType,AName,value);
  return value.isValid() ? value : QTextBrowser::loadResource(AType,AName);
}
