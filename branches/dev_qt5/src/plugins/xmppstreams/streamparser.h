#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QDomDocument>
#include <QXmlStreamReader>
#include <definitions/namespaces.h>
#include <utils/xmpperror.h>

class StreamParser :
	public QObject
{
	Q_OBJECT;
public:
	StreamParser(QObject *AParent = NULL);
	~StreamParser();
	void parseData(const QByteArray &AData);
	void restart();
signals:
	void opened(const QDomElement &AElem);
	void element(const QDomElement &AElem);
	void error(const XmppError &AError);
	void closed();
private:
	int FLevel;
	QDomText FElemSpace;
	QDomElement FRootElem;
	QDomElement FCurrentElem;
	QXmlStreamReader FReader;
};

#endif // STREAMPARSER_H
