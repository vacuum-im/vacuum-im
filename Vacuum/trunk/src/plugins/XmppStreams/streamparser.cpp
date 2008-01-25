#include <QtDebug>
#include "streamparser.h"

StreamParser::StreamParser(QObject *AParent) : QObject(AParent)
{
  FReader.setFeature("http://xml.org/sax/features/namespaces", false);
  FReader.setFeature("http://xml.org/sax/features/namespace-prefixes", true);
  FReader.setContentHandler(this);
}

StreamParser::~StreamParser()
{

}

bool StreamParser::parceData(const QString &AData)
{
  FSource.setData(AData); 
  if (!FContinue)
  { 
    FContinue = true;
    return FReader.parse(&FSource, true);
  }
  return FReader.parseContinue();
}

void StreamParser::clear() 
{
  FDoc.clear();
  FLevel=0;
  FContinue=false;
  FSource.setData(QByteArray()); 
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
    emit open(FDoc.documentElement());
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

  if (FLevel < 0) 
    return false;

  if (FLevel == 0)
  {
    emit closed();
    clear();
  }
  else if (FLevel == 1)
  {
    emit element(FDoc.documentElement());
  }
  else 
  {
    FElement = FElement.parentNode().toElement();
  }

  return true;
}

bool StreamParser::characters(const QString &AText)
{
  FElement.appendChild(FDoc.createTextNode(AText));   
  return true;
}

bool StreamParser::fatalError(const QXmlParseException &AException)
{
  emit error(AException.message());
  return false;
}
