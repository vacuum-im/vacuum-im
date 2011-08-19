#include "streamparser.h"

StreamParser::StreamParser(QObject *AParent) : QObject(AParent)
{
	FReader.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("ns20","strange-yandex-bug-20"));
	FReader.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("ns21","strange-yandex-bug-21"));
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
			FLevelNS.clear();
			FLevelNS.push(NS_JABBER_CLIENT);
		}
		else if (FReader.isStartElement())
		{
			QString nsURI = FReader.namespaceUri().toString();
			QString elemName = FReader.qualifiedName().toString();
			QDomElement newElement = FLevelNS.top()!=nsURI ? doc.createElementNS(nsURI,elemName) : doc.createElement(elemName);
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
				FLevelNS.pop();
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
