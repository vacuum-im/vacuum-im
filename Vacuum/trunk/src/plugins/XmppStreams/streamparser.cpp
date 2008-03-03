#include <QtDebug>
#include "streamparser.h"

StreamParser::StreamParser(QObject *AParent) : QObject(AParent)
{
  FReader.setFeature("http://xml.org/sax/features/namespaces", false);
  FReader.setFeature("http://xml.org/sax/features/namespace-prefixes", false);
  FReader.setContentHandler(this);
  FReader.setErrorHandler(this);
}

StreamParser::~StreamParser()
{

}

void StreamParser::parseData(const QByteArray &AData)
{
  FSource.setData(AData);
  bool parsed = FContinue ? FReader.parseContinue() : FReader.parse(&FSource, true);
  if (!parsed)
  {
    qDebug() << "\nERROR PARSING DATA:" << AData;
    emit error(FErrorString);
  }
}

void StreamParser::restart() 
{
  FContinue = false;
}

bool StreamParser::startDocument()
{
  FLevel=0;
  FDoc.clear();
  FContinue = true;
  FErrorString = tr("Unknown Parser Exception");
  return true;
}

bool StreamParser::startElement(const QString &/*ANamespaceURI*/, const QString &/*ALocalName*/, 
                                const QString &AQName, const QXmlAttributes &AAttr)
{
  QString xmlns = FLevel==0 ? "xmlns:stream" : "xmlns";
  QString nsAttr = AAttr.value(xmlns);

  QDomElement newElement = nsAttr.isEmpty() ? FDoc.createElement(AQName) : FDoc.createElementNS(nsAttr,AQName);

  for (int i=0; i < AAttr.count(); i++)
    if (AAttr.qName(i) != xmlns) 
      newElement.setAttribute(AAttr.qName(i), AAttr.value(i)); 

  if (FLevel == 0)
  {
    FElement = newElement;
    FDoc.appendChild(FElement);
    emit opened(FDoc.documentElement());
  } 
  else if (FLevel == 1)
  { 
    FDoc.removeChild(FElement); 
    FElement = FDoc.appendChild(newElement).toElement();
  }
  else
  {
    FElement = FElement.appendChild(newElement).toElement(); 
  } 
  FLevel++;
  return true;
}

bool StreamParser::endElement(const QString &/*ANamespaceURI*/, const QString &/*ALocalName*/, const QString &/*AQName*/)
{
  FLevel--;
  if (FLevel == 0)
    emit closed();
  else if (FLevel == 1)
    emit element(FDoc.documentElement());
  else if (FLevel > 0)
    FElement = FElement.parentNode().toElement();
  else 
  {
    FErrorString = tr("Unexpected end of element");
    return false;
  }
  return true;
}

bool StreamParser::characters(const QString &AText)
{
  FElement.appendChild(FDoc.createTextNode(AText));
  return true;
}

bool StreamParser::endDocument()
{
  FContinue = false;
  return true;
}

bool StreamParser::error(const QXmlParseException &AException)
{
  FErrorString = AException.message();
  return false;
}

bool StreamParser::warning(const QXmlParseException &AException)
{
  qDebug() << "PARSER WARNING:" << AException.message();
  return true;
}

bool StreamParser::fatalError(const QXmlParseException &AException)
{
  FErrorString = AException.message();
  return false;
}

