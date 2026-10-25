#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QString>
#include <QUrl>

#include <functional>

#define IV_MAX_CONCURRENT_DOWNLOADS  2
// The download is aborted after this time without any data (a stall), not
// after a total time: a slow but moving transfer is not interrupted.
#define IV_DOWNLOAD_TIMEOUT_MS       15000

// The images referenced by the messages are downloaded here. The downloads of
// the same url are made only once, all the requests of it are served with the
// same data.
class NetworkManager : public QObject
{
	Q_OBJECT
public:
	// The handler is called with the downloaded data, the data is empty when
	// the download has failed
	typedef std::function<void(const QByteArray &AData)> DataHandler;
	explicit NetworkManager(QObject *AParent = nullptr);
	// Downloads the url, the handler is called when the download is finished.
	// The download is aborted when it is bigger than the given limit.
	void request(const QString &AUrlStr, const QUrl &AUrl, qint64 AMaxBytes, const DataHandler &AHandler);
	// The host of the url must not be requested
	bool isHostBlocked(const QUrl &AUrl) const;
private:
	bool isAddressBlocked(const QHostAddress &AAddress) const;
	// The address is in a reserved range, that is commonly used by the tunnels
	// and the proxies for the fake addresses of the names
	bool isReservedAddress(const QHostAddress &AAddress) const;
	void doNetworkRequest(const QString &AUrlStr, const QUrl &AUrl, qint64 AMaxBytes, const DataHandler &AHandler);
	void startRequestForUrl(const QString &AUrlStr, const QUrl &AUrl);
	void issueNetworkRequest(const QString &AUrlStr, const QUrl &AUrl);
	void failDownload(const QString &AUrlStr, const QUrl &AFailedUrl);
	void handleReplyFinished(QNetworkReply *AReply, const QString &AUrlStr);
	void startQueuedDownloads();
private:
	struct InFlightDownload
	{
		QNetworkReply *reply = nullptr;
		QString urlStr;
		QUrl currentUrl;
		QList<DataHandler> waiters;
		qint64 received = 0;
		qint64 maxBytes = 0;
		QByteArray data;
		int redirects = 0;
		quint64 dnsGeneration = 0;
		bool tooLarge = false;
	};
	struct QueuedRequest
	{
		QString urlStr;
		QUrl url;
		qint64 maxBytes = 0;
		DataHandler handler;
	};
private:
	QNetworkAccessManager *FNetworkManager;
	int FActiveDownloads;
	QHash<QString, InFlightDownload> FInFlight;
	QQueue<QueuedRequest> FQueued;
};

#endif // NETWORKMANAGER_H
