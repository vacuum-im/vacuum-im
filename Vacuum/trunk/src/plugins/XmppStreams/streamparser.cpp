#include <QtDebug>
#include "streamparser.h"

StreamParser::StreamParser(QObject *parent)
: QObject(parent)
{
  FReader.setFeature("http://xml.org/sax/features/namespaces", false);
  FReader.setFeature("http://xml.org/sax/features/namespace-prefixes", true);
  FReader.setContentHandler(this);
}

StreamParser::~StreamParser()
{
  qDebug() << "~StreamParser";
}

bool StreamParser::parceData(const QString &data)
{
  FSource.setData(data); 
  if (!FContinue)
  { 
    FContinue = true;
    return FReader.parse(&FSource, true);
  };
  return FReader.parseContinue();
}

void StreamParser::clear() 
{
  FDoc.clear();
  FLevel=0;
  FContinue=false;
  FSource.setData(QByteArray()); 
}

bool StreamParser::startElement(const QString &namespaceURI, 
                                const QString &localName, 
                                const QString &qName, 
                                const QXmlAttributes &atts)
{
  Q_UNUSED(namespaceURI);
  Q_UNUSED(localName);

  QString xmlns;
  if (FLevel == 0)
    xmlns = "xmlns:stream";
  else
    xmlns = "xmlns";
  QString nsAtt = atts.value(xmlns);

  QDomElement newElement;
  if (nsAtt.isEmpty())
    newElement = FDoc.createElement(qName);
  else
    newElement = FDoc.createElementNS(nsAtt,qName);

  for (int i=0; i < atts.count(); i++)
    if (atts.qName(i) != xmlns) 
      newElement.setAttribute(atts.qName(i), atts.value(i)); 

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
  }; 
  FLevel++;
  return true;
}

bool StreamParser::endElement(const QString &namespaceURI, 
                              const QString &localName, 
                              const QString &qName)
{
  Q_UNUSED(namespaceURI);
  Q_UNUSED(localName);
  Q_UNUSED(qName);

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
  };

  return true;
}

bool StreamParser::characters(const QString &ch)
{
  FElement.appendChild(FDoc.createTextNode(ch));   
  return true;
}

bool StreamParser::fatalError(const QXmlParseException &exception)
{
  emit error(exception.message());
  return false;
}
