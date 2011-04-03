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
			QMap<QString, QString> attributes;
			foreach(QXmlStreamAttribute attribute, FReader.attributes())
				attributes.insert(attribute.qualifiedName().toString(), attribute.value().toString());

			QString nsURI = attributes.take("xmlns");
			QString elemName = FReader.qualifiedName().toString();

			QDomElement newElement = !nsURI.isEmpty() ? doc.createElementNS(nsURI,elemName) : doc.createElement(elemName);
			for (QMap<QString, QString>::const_iterator it = attributes.constBegin(); it!=attributes.constEnd(); it++)
				newElement.setAttribute(it.key(),it.value());

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
	FReader.setNamespaceProcessing(false);
}
