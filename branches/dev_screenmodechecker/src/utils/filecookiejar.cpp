#include "filecookiejar.h"

#include <QFile>

FileCookieJar::FileCookieJar(const QString &AFile, QObject *AParent) : QNetworkCookieJar(AParent)
{
	FFile = AFile;
	loadCookies(FFile);
}

FileCookieJar::~FileCookieJar()
{
	saveCookies(FFile);
}

bool FileCookieJar::loadCookies(const QString &AFile)
{
	QFile file(AFile);
	if (file.open(QFile::ReadOnly))
	{
		QList<QNetworkCookie> cookies;
		QList<QByteArray> cookieItems = file.readAll().split('\n');
		foreach(const QByteArray &cookieItem, cookieItems)
			cookies.append(QNetworkCookie::parseCookies(cookieItem));
		setAllCookies(cookies);
		return true;
	}
	return false;
}

bool FileCookieJar::saveCookies(const QString &AFile) const
{
	QFile file(AFile);
	if (file.open(QFile::WriteOnly|QIODevice::Truncate))
	{
		QByteArray cookies;
		foreach (const QNetworkCookie &cookie, allCookies())
			cookies += cookie.toRawForm()+'\n';
		file.write(cookies);
		file.close();
		return true;
	}
	return false;
}
