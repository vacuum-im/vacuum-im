#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QXmlSimpleReader>

class StreamParser : 
  public QObject,
  private QXmlDefaultHandler
{
  Q_OBJECT

public:
  StreamParser(QObject *parent);
  ~StreamParser();

  virtual bool parceData(const QString &data);
  virtual void clear();
signals:
  void open(const QDomElement &);
  void element(const QDomElement &);
  void error(const QString &);
  void closed();
private:
  virtual bool startElement(
    const QString &namespaceURI,
    const QString &localName,
    const QString &qName,
    const QXmlAttributes &atts);
  virtual bool endElement(
    const QString &namespaceURI,
    const QString &localName,
    const QString &qName);
  virtual bool characters(const QString &ch);
  virtual bool fatalError(const QXmlParseException &exception);
private:
  bool FContinue;
  int FLevel;
  QDomDocument FDoc;
  QDomElement FElement;
  QXmlSimpleReader FReader;
  QXmlInputSource FSource;
};

#endif // STREAMPARSER_H
