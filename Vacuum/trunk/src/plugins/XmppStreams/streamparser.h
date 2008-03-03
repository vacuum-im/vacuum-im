#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QXmlSimpleReader>

class StreamParser : 
  public QObject,
  public QXmlDefaultHandler
{
  Q_OBJECT;
public:
  StreamParser(QObject *AParent = NULL);
  ~StreamParser();
  void parseData(const QByteArray &AData);
  void restart();
signals:
  void opened(const QDomElement &AElement);
  void element(const QDomElement &AElement);
  void error(const QString &AError);
  void closed();
private:
  //QXmlContentHandler
  virtual bool startDocument();
  virtual bool startElement(const QString &ANamespaceURI, const QString &ALocalName,const QString &AQName, const QXmlAttributes &AAttr);
  virtual bool endElement(const QString &ANamespaceURI, const QString &ALocalName, const QString &AQName);
  virtual bool characters(const QString &AText);
  virtual bool endDocument();
  //QXmlErrorHandler
  virtual bool error(const QXmlParseException &AException);
  virtual bool warning (const QXmlParseException & exception);
  virtual bool fatalError(const QXmlParseException &AException);
private:
  bool FContinue;
  int FLevel;
  QString FErrorString;
  QDomDocument FDoc;
  QDomElement FElement;
  QXmlSimpleReader FReader;
  QXmlInputSource FSource;
};

#endif // STREAMPARSER_H
