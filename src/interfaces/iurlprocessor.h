#ifndef IURLPROCESSOR_H
#define IURLPROCESSOR_H

#include <QNetworkReply>
#include <QNetworkAccessManager>

#define URLPROCESSOR_UUID QUuid("{c2d1eba4-a18d-bf31-da24-bc61d33f3205}")

class IUrlHandler
{
public:
	virtual QNetworkReply *request(QNetworkAccessManager::Operation AOperation, const QNetworkRequest &ARequest, QIODevice *AOutData=NULL) = 0;
};

class IUrlProcessor
{
public:
	virtual QObject *instance() = 0;
	virtual QNetworkAccessManager *networkAccessManager() = 0;
	virtual bool registerUrlHandler(const QString &AScheme, IUrlHandler *AUrlHandler) = 0;
};

Q_DECLARE_INTERFACE(IUrlHandler, "Vacuum.Plugin.IUrlHandler/1.1")
Q_DECLARE_INTERFACE(IUrlProcessor, "Vacuum.Plugin.IUrlProcessor/1.1")

#endif	//IURLPROCESSOR_H
