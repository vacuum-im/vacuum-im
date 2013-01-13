#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QStack>
#include <QDomDocument>
#include <QXmlStreamReader>

class StreamParser : 
  public QObject
{
  Q_OBJECT;
public:
  StreamParser(QObject *AParent = NULL);
  ~StreamParser();
  void parseData(const QByteArray &AData);
  void restart();
signals:
  void opened(QDomElement AElem);
  void element(QDomElement AElem);
  void error(const QString &AError);
  void closed();
private:
  int FLevel;
  QStack<QString> FLevelNS;
  QDomDocument FDoc;
  QDomElement FElement;
  QXmlStreamReader FReader;
};

#endif // STREAMPARSER_H
