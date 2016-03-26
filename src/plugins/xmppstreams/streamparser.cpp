#include "streamparser.h"

#include <QMap>
#include <definitions/namespaces.h>
#include <utils/logger.h>

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
			FStanzaElem = FCurrentElem = QDomElement();
		}
		else if (FReader.isStartElement())
		{
			QDomElement newElem = doc.createElementNS(FReader.namespaceUri().toString(),FReader.qualifiedName().toString());
			
			foreach(const QXmlStreamNamespaceDeclaration &ns, FReader.namespaceDeclarations())
			{
				if (ns.prefix() != FReader.prefix())
				{
					if (!ns.prefix().isEmpty())
						newElem.setAttribute(QString("xmlns:%1").arg(ns.prefix().toString()), ns.namespaceUri().toString());
					else
						newElem.setAttribute(QString("xmlns"), ns.namespaceUri().toString());
				}
			}

			foreach(const QXmlStreamAttribute &attribute, FReader.attributes())
			{
				if (!attribute.namespaceUri().isEmpty())
					newElem.setAttributeNS(attribute.namespaceUri().toString(),attribute.qualifiedName().toString(),attribute.value().toString());
				else
					newElem.setAttribute(attribute.qualifiedName().toString(),attribute.value().toString());
			}

			FLevel++;
			if (FLevel == 1)
			{
				emit opened(newElem);
			}
			else if (FLevel == 2)
			{
				FStanzaElem = newElem;
				FCurrentElem = FStanzaElem;
			}
			else
			{
				FCurrentElem.appendChild(newElem);
				FCurrentElem = newElem;
			}

			FElemSpace = QDomText();
		}
		else if (FReader.isCharacters())
		{
			if (FReader.isCDATA())
				FCurrentElem.appendChild(doc.createCDATASection(FReader.text().toString()));
			else if (FReader.isWhitespace())
				FElemSpace = doc.createTextNode(FReader.text().toString());
			else
				FCurrentElem.appendChild(doc.createTextNode(FReader.text().toString()));
		}
		else if (FReader.isEndElement())
		{
			if (!FElemSpace.isNull() && !FCurrentElem.hasChildNodes())
				FCurrentElem.appendChild(FElemSpace);
			FElemSpace = QDomText();

			FLevel--;
			if (FLevel > 1)
				FCurrentElem = FCurrentElem.parentNode().toElement();
			else if (FLevel == 1)
				emit element(FStanzaElem);
			else if (FLevel == 0)
				emit closed();
		}
	}

	if (FReader.hasError() && FReader.error()!=QXmlStreamReader::PrematureEndOfDocumentError)
	{
		LOG_ERROR(QString("Failed to parse XML data: %1\n%2").arg(FReader.errorString(),QString::fromUtf8(AData)));
		emit error(XmppStreamError(XmppStreamError::EC_NOT_WELL_FORMED,FReader.errorString()));
	}
}

void StreamParser::restart()
{
	FReader.clear();
	FReader.setNamespaceProcessing(true);
}
