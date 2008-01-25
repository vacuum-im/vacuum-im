#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QXmlSimpleReader>

class StreamParser : 
  public QObject,
  private QXmlDefaultHandler
{
  Q_OBJECT;
public:
  StreamParser(QObject *AParent = NULL);
  ~StreamParser();
  virtual bool parceData(const QString &AData);
  virtual void clear();
signals:
  void open(const QDomElement &AElement);
  void element(const QDomElement &AElement);
  void error(const QString &AElement);
  void closed();
private:
  virtual bool startElement(const QString &ANamespaceURI, const QString &ALocalName,const QString &AQName, const QXmlAttributes &AAttr);
  virtual bool endElement(const QString &ANamespaceURI, const QString &ALocalName, const QString &AQName);
  virtual bool characters(const QString &AText);
  virtual bool fatalError(const QXmlParseException &AException);
private:
  bool FContinue;
  int FLevel;
  QDomDocument FDoc;
  QDomElement FElement;
  QXmlSimpleReader FReader;
  QXmlInputSource FSource;
};

#endif // STREAMPARSER_H
