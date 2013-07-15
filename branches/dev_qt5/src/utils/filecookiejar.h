#ifndef FILECOOKIEJAR_H
#define FILECOOKIEJAR_H

#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include "utilsexport.h"

class UTILS_EXPORT FileCookieJar : 
	public QNetworkCookieJar
{
	Q_OBJECT;
public:
	FileCookieJar(const QString &AFile, QObject *AParent=NULL);
	~FileCookieJar();
protected:
	bool loadCookies(const QString &AFile);
	bool saveCookies(const QString &AFile) const;
private:
	QString FFile;
};

#endif // FILECOOKIEJAR_H
