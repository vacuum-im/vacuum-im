#include "streamparser.h"

StreamParser::StreamParser(QObject *AParent) : QObject(AParent)
{
  FReader.setNamespaceProcessing(true);
}

StreamParser::~StreamParser()
{

}

void StreamParser::parseData(const QByteArray &AData)
{
  FReader.addData(AData);
  while (!FReader.atEnd())
  {
    FReader.readNext();
    if (FReader.isStartDocument())
    {
      FLevel = 0;
      FDoc.clear();
      FLevelNS.clear();
      FLevelNS.push("");
    }
    else if (FReader.isStartElement())
    {
      QString nsURI = FReader.namespaceUri().toString();
      QString elemName = FReader.qualifiedName().toString();
      QDomElement newElement = FLevelNS.top()!=nsURI ? FDoc.createElementNS(nsURI,elemName) : FDoc.createElement(elemName);
      FLevelNS.push(nsURI);

      foreach(QXmlStreamAttribute attribute, FReader.attributes())
      {
        QString attrNS = attribute.namespaceUri().toString();
        if (attrNS.isEmpty())
          newElement.setAttribute(attribute.name().toString(),attribute.value().toString());
        else
          newElement.setAttributeNS(attrNS,attribute.qualifiedName().toString(),attribute.value().toString());
      }

      FLevel++;
      if (FLevel == 1)
      {
        FElement = newElement;
        FDoc.appendChild(FElement);
        emit opened(FDoc.documentElement());
      } 
      else if (FLevel == 2)
      { 
        FDoc.removeChild(FElement); 
        FElement = FDoc.appendChild(newElement).toElement();
      }
      else
      {
        FElement = FElement.appendChild(newElement).toElement(); 
      } 
    }
    else if (FReader.isCharacters())
    {
      if (!FReader.isCDATA() && !FReader.isWhitespace())
        FElement.appendChild(FDoc.createTextNode(FReader.text().toString()));
    }
    else if (FReader.isEndElement())
    {
      FLevel--;
      if (FLevel == 0)
        emit closed();
      else if (FLevel == 1)
       emit element(FDoc.documentElement());
      else if (FLevel > 1)
        FElement = FElement.parentNode().toElement();
      FLevelNS.pop();
    }
  }

  if (FReader.hasError() && FReader.error()!=QXmlStreamReader::PrematureEndOfDocumentError)
  {
    emit error(FReader.errorString());
  }
}

void StreamParser::restart() 
{
  FReader.clear();
}
