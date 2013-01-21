#ifndef STREAMPARSER_H
#define STREAMPARSER_H

#include <QStack>
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
	void opened(QDomElement AElem);
	void element(QDomElement AElem);
	void error(const XmppError &AError);
	void closed();
private:
	int FLevel;
	QDomElement FRootElem;
	QDomElement FCurrentElem;
	QXmlStreamReader FReader;
};

#endif // STREAMPARSER_H
