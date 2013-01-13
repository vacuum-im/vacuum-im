#include "streamparser.h"

#include <QMap>

StreamParser::StreamParser(QObject *AParent) : QObject(AParent)
{
	restart();
}

StreamParser::~StreamParser()
{

}

void StreamParser::parseData(const QByteArray &AData)
{
	static QDomDocument doc;

	FReader.addData(AData);
	while (!FReader.atEnd())
	{
		FReader.readNext();
		if (FReader.isStartDocument())
		{
			FLevel = 0;
		}
		else if (FReader.isStartElement())
		{
			QDomElement newElement = doc.createElementNS(FReader.namespaceUri().toString(),FReader.qualifiedName().toString());
			foreach(QXmlStreamAttribute attribute, FReader.attributes())
			{
				QString attrNs = attribute.namespaceUri().toString();
				if (!attrNs.isEmpty())
					newElement.setAttributeNS(attrNs,attribute.qualifiedName().toString(),attribute.value().toString());
				else
					newElement.setAttribute(attribute.qualifiedName().toString(),attribute.value().toString());
			}

			FLevel++;
			if (FLevel == 1)
			{
				emit opened(newElement);
			}
			else if (FLevel == 2)
			{
				FRootElem = newElement;
				FCurrentElem = FRootElem;
			}
			else
			{
				FCurrentElem.appendChild(newElement);
				FCurrentElem = newElement;
			}
		}
		else if (FReader.isCharacters())
		{
			if (!FReader.isCDATA() && !FReader.isWhitespace())
				FCurrentElem.appendChild(doc.createTextNode(FReader.text().toString()));
		}
		else if (FReader.isEndElement())
		{
			FLevel--;
			if (FLevel == 0)
				emit closed();
			else if (FLevel == 1)
				emit element(FRootElem);
			else if (FLevel > 1)
				FCurrentElem = FCurrentElem.parentNode().toElement();
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
	FReader.setNamespaceProcessing(true);
}
