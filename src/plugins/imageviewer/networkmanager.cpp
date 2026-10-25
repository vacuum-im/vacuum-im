#include "networkmanager.h"

#include <QAbstractSocket>
#include <QHostInfo>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>

#include <definitions/optionvalues.h>

#include <utils/logger.h>
#include <utils/options.h>

namespace
{
	// The reply is closed when it is finished or aborted, reading it then
	// gives a warning and no data.
	QByteArray readReplyData(QNetworkReply *AReply)
	{
		return AReply->isOpen() ? AReply->readAll() : QByteArray();
	}
} // namespace

NetworkManager::NetworkManager(QObject *AParent)
	: QObject(AParent), FNetworkManager(new QNetworkAccessManager(this)), FActiveDownloads(0)
{
}

bool NetworkManager::isHostBlocked(const QUrl &AUrl) const
{
	if (!Options::node(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS).value().toBool())
		return false;

	const QString host = AUrl.host().toLower();
	if (host.isEmpty() || AUrl.userInfo().size() > 0 ||
		host == QStringLiteral("localhost") || host.endsWith(QStringLiteral(".localhost")) ||
		host == QStringLiteral("localhost.localdomain") || host.endsWith(QStringLiteral(".local")) ||
		host.endsWith(QStringLiteral(".internal")))
		return true;

	QHostAddress address;
	if (address.setAddress(host))
		return isAddressBlocked(address);
	static const QRegularExpression numericHostRegex(QStringLiteral(R"REGEX(^(?:[0-9.]+|0x[0-9a-f]+)$)REGEX"),
													 QRegularExpression::CaseInsensitiveOption);
	if (host.contains(QLatin1Char(':')) || host.startsWith(QLatin1Char('.')) ||
		host.endsWith(QLatin1Char('.')) || numericHostRegex.match(host).hasMatch())
		return true;

	// Names are checked asynchronously before a request is sent.
	return false;
}

bool NetworkManager::isAddressBlocked(const QHostAddress &AAddress) const
{
	if (!Options::node(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS).value().toBool())
		return false;

	if (AAddress.protocol() == QAbstractSocket::IPv4Protocol)
	{
		const quint32 ip = AAddress.toIPv4Address();
		const quint8 b0 = (ip >> 24) & 0xff;
		const quint8 b1 = (ip >> 16) & 0xff;
		const quint8 b2 = (ip >> 8) & 0xff;
		const quint8 b3 = ip & 0xff;
		if (b0 == 0 || b0 == 10 || b0 == 127 || b0 >= 224)
			return true;
		if (b0 == 169 && b1 == 254)
			return true;
		if (b0 == 172 && b1 >= 16 && b1 <= 31)
			return true;
		if (b0 == 192 && b1 == 168)
			return true;
		if (b0 == 100 && b1 >= 64 && b1 <= 127)
			return true;
		if (b0 == 192 && b1 == 0 && b2 == 0)
			return true;
		if (b0 == 192 && b1 == 0 && b2 == 2)
			return true;
		if (b0 == 198 && (b1 == 18 || b1 == 19))
			return true;
		if (b0 == 198 && b1 == 51 && b2 == 100)
			return true;
		if (b0 == 203 && b1 == 0 && b2 == 113)
			return true;
		if (b0 == 192 && b1 == 88 && b2 == 99)
			return true;
		if (b0 == 255 && b1 == 255 && b2 == 255 && b3 == 255)
			return true;
		return false;
	}

	if (AAddress.protocol() == QAbstractSocket::IPv6Protocol)
	{
		const Q_IPV6ADDR ip = AAddress.toIPv6Address();
		// IPv4-mapped IPv6 must inherit IPv4 restrictions.
		const bool v4mapped = ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0 &&
							  ip[4] == 0 && ip[5] == 0 && ip[6] == 0 && ip[7] == 0 &&
							  ip[8] == 0 && ip[9] == 0 && ip[10] == 0xff && ip[11] == 0xff;
		if (v4mapped)
		{
			const quint32 v4 = (quint32(ip[12]) << 24) | (quint32(ip[13]) << 16) |
							   (quint32(ip[14]) << 8) | quint32(ip[15]);
			return isAddressBlocked(QHostAddress(v4));
		}
		// Only global-unicast 2000::/3 is allowed. This rejects unspecified,
		// loopback, link-local, ULA, multicast, IPv4-compatible and special-use ranges.
		if ((ip[0] & 0xe0) != 0x20)
			return true;
		// Conservative rejection of IETF special-use 2001::/23, documentation,
		// and 6to4 addresses, which can embed IPv4 destinations.
		if (ip[0] == 0x20 && ip[1] == 0x01 && (ip[2] & 0xfe) == 0x00)
			return true;
		if (ip[0] == 0x20 && ip[1] == 0x01 && ip[2] == 0x0d && ip[3] == 0xb8)
			return true;
		if (ip[0] == 0x20 && ip[1] == 0x02)
			return true;
		return false;
	}

	return true;
}

// The addresses of the reserved ranges. They are not reachable in the public
// internet, but the tunnels and the proxies map the names of the public hosts
// to them, so a name resolved to them is not a private one.
bool NetworkManager::isReservedAddress(const QHostAddress &AAddress) const
{
	if (AAddress.protocol() == QAbstractSocket::IPv4Protocol)
	{
		const quint32 ip = AAddress.toIPv4Address();
		const quint8 b0 = (ip >> 24) & 0xff;
		const quint8 b1 = (ip >> 16) & 0xff;
		const quint8 b2 = (ip >> 8) & 0xff;
		// 198.18.0.0/15, the benchmarking range, used for the fake addresses
		if (b0 == 198 && (b1 == 18 || b1 == 19))
			return true;
		// the documentation ranges
		if (b0 == 192 && b1 == 0 && b2 == 2)
			return true;
		if (b0 == 198 && b1 == 51 && b2 == 100)
			return true;
		if (b0 == 203 && b1 == 0 && b2 == 113)
			return true;
		return false;
	}

	if (AAddress.protocol() == QAbstractSocket::IPv6Protocol)
	{
		const Q_IPV6ADDR ip = AAddress.toIPv6Address();
		// 2001:db8::/32, the documentation range
		if (ip[0] == 0x20 && ip[1] == 0x01 && ip[2] == 0x0d && ip[3] == 0xb8)
			return true;
		// 2002::/16, 6to4
		if (ip[0] == 0x20 && ip[1] == 0x02)
			return true;
		return false;
	}

	return false;
}

void NetworkManager::request(const QString &AUrlStr, const QUrl &AUrl, qint64 AMaxBytes, const DataHandler &AHandler)
{
	auto it = FInFlight.find(AUrlStr);
	if (it != FInFlight.end())
	{
		// The image is already being downloaded, its data will serve this request too.
		it->waiters.append(AHandler);
		return;
	}

	if (FActiveDownloads >= IV_MAX_CONCURRENT_DOWNLOADS)
	{
		FQueued.enqueue({AUrlStr, AUrl, AMaxBytes, AHandler});
		return;
	}

	doNetworkRequest(AUrlStr, AUrl, AMaxBytes, AHandler);
}

void NetworkManager::doNetworkRequest(const QString &AUrlStr, const QUrl &AUrl, qint64 AMaxBytes, const DataHandler &AHandler)
{
	InFlightDownload entry;
	entry.reply = nullptr;
	entry.urlStr = AUrlStr;
	entry.currentUrl = AUrl;
	entry.waiters.append(AHandler);
	entry.received = 0;
	entry.maxBytes = AMaxBytes;
	entry.redirects = 0;
	entry.tooLarge = false;
	FInFlight.insert(AUrlStr, entry);
	++FActiveDownloads;
	startRequestForUrl(AUrlStr, AUrl);
}

void NetworkManager::startRequestForUrl(const QString &AUrlStr, const QUrl &AUrl)
{
	auto it = FInFlight.find(AUrlStr);
	if (it == FInFlight.end())
		return;
	if (!AUrl.isValid() ||
		(AUrl.scheme() != QStringLiteral("http") && AUrl.scheme() != QStringLiteral("https")) ||
		AUrl.host().isEmpty() || !AUrl.userInfo().isEmpty() || AUrl.port(-1) == 0 || isHostBlocked(AUrl))
	{
		failDownload(AUrlStr, AUrl);
		return;
	}
	it->currentUrl = AUrl;
	const quint64 dnsGeneration = ++it->dnsGeneration;

	// The name is validated here, but QNetworkAccessManager resolves it again
	// when the request is sent: a dns rebinding attacker can answer this
	// lookup with a public address and the request lookup with a private one.
	// Connecting to the validated address instead of the name would break tls,
	// the certificate and sni are bound to the name, so the check is a
	// mitigation, not a guarantee.
	QHostAddress literalAddress;
	if (Options::node(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS).value().toBool() &&
		!literalAddress.setAddress(AUrl.host()))
	{
		QTimer::singleShot(IV_DOWNLOAD_TIMEOUT_MS, this, [this, AUrlStr, AUrl, dnsGeneration]()
						   {
			auto current = FInFlight.find(AUrlStr);
			if (current != FInFlight.end() && !current->reply && current->currentUrl == AUrl &&
			    current->dnsGeneration == dnsGeneration)
				failDownload(AUrlStr, AUrl); });
		QHostInfo::lookupHost(AUrl.host(), this, [this, AUrlStr, AUrl, dnsGeneration](const QHostInfo &info)
							  {
			auto current = FInFlight.find(AUrlStr);
			if (current == FInFlight.end() || current->reply || current->dnsGeneration != dnsGeneration) return;
			if (info.error() != QHostInfo::NoError || info.addresses().isEmpty())
			{
				LOG_INFO(QString("ImageViewer: %1 is not resolved").arg(AUrlStr));
				failDownload(AUrlStr, AUrl);
				return;
			}
			bool allowed = false;
			for (const QHostAddress &address : info.addresses())
			{
				if (address == QHostAddress(QHostAddress::AnyIPv4) ||
					address == QHostAddress(QHostAddress::AnyIPv6))
				{
					// The unspecified address tells nothing about the host.
					continue;
				}
				if (!isAddressBlocked(address))
				{
					allowed = true;
					continue;
				}
				if (!isReservedAddress(address))
				{
					// A private address, the name must not be requested.
					LOG_INFO(QString("ImageViewer: %1 is resolved to the private address %2").arg(AUrlStr, address.toString()));
					failDownload(AUrlStr, AUrl);
					return;
				}
			}
			if (!allowed)
			{
				// The name is mapped to the reserved addresses by a tunnel or a
				// proxy, the request is made anyway.
				LOG_INFO(QString("ImageViewer: %1 is resolved to the reserved addresses only").arg(AUrlStr));
			}
					issueNetworkRequest(AUrlStr, AUrl); });
		return;
	}
	if (Options::node(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS).value().toBool() && isAddressBlocked(literalAddress))
	{
		failDownload(AUrlStr, AUrl);
		return;
	}
	issueNetworkRequest(AUrlStr, AUrl);
}

void NetworkManager::issueNetworkRequest(const QString &AUrlStr, const QUrl &AUrl)
{
	auto it = FInFlight.find(AUrlStr);
	if (it == FInFlight.end())
		return;
	QNetworkRequest request(AUrl);
	request.setRawHeader("User-Agent", "Vacuum-IM/1.0");
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
	QNetworkReply *reply = FNetworkManager->get(request);
	it->reply = reply;
	it->currentUrl = AUrl;
	it->received = 0;
	it->data.clear();
	it->tooLarge = false;
	const qint64 maxBytes = it->maxBytes;

	connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, AUrlStr, maxBytes]()
			{
		auto current = FInFlight.find(AUrlStr);
		if (current == FInFlight.end() || current->reply != reply) return;
		const QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
		if (length.isValid() && length.toLongLong() > maxBytes)
		{
			current->tooLarge = true;
			current->data.clear();
			reply->abort();
		} });
	connect(reply, &QNetworkReply::readyRead, this, [this, reply, AUrlStr, maxBytes]()
			{
		auto current = FInFlight.find(AUrlStr);
		if (current == FInFlight.end() || current->reply != reply || current->tooLarge)
		{
			readReplyData(reply);
			return;
		}
		const QByteArray chunk = readReplyData(reply);
		if (current->received + chunk.size() > maxBytes)
		{
			current->tooLarge = true;
			current->data.clear();
			reply->abort();
			return;
		}
		current->received += chunk.size();
		current->data.append(chunk); });
	connect(reply, &QNetworkReply::finished, this, [this, reply, AUrlStr]()
			{ handleReplyFinished(reply, AUrlStr); });
	// The download is aborted when no data arrives for the timeout time: a
	// slow but moving transfer is allowed to run as long as it needs, while a
	// stalled one is given up. The timer is restarted on every progress.
	QTimer *stallTimer = new QTimer(reply);
	stallTimer->setSingleShot(true);
	connect(stallTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
	connect(reply, &QNetworkReply::metaDataChanged, stallTimer, [stallTimer]()
			{ stallTimer->start(IV_DOWNLOAD_TIMEOUT_MS); });
	connect(reply, &QNetworkReply::readyRead, stallTimer, [stallTimer]()
			{ stallTimer->start(IV_DOWNLOAD_TIMEOUT_MS); });
	connect(reply, &QNetworkReply::finished, stallTimer, &QTimer::stop);
	stallTimer->start(IV_DOWNLOAD_TIMEOUT_MS);
}

void NetworkManager::failDownload(const QString &AUrlStr, const QUrl &AFailedUrl)
{
	Q_UNUSED(AFailedUrl);
	auto it = FInFlight.find(AUrlStr);
	if (it == FInFlight.end())
		return;
	const InFlightDownload entry = FInFlight.take(AUrlStr);
	FActiveDownloads = qMax(0, FActiveDownloads - 1);
	LOG_INFO(QString("ImageViewer: the download of %1 has failed").arg(AUrlStr));
	for (const DataHandler &handler : entry.waiters)
		handler(QByteArray());
	startQueuedDownloads();
}

void NetworkManager::handleReplyFinished(QNetworkReply *AReply, const QString &AUrlStr)
{
	auto it = FInFlight.find(AUrlStr);
	if (it == FInFlight.end() || it->reply != AReply)
	{
		AReply->deleteLater();
		return;
	}

	const QVariant redirectAttribute = AReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	const int statusCode = AReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (!it->tooLarge && redirectAttribute.isValid() && statusCode >= 300 && statusCode < 400)
	{
		QUrl nextUrl = it->currentUrl.resolved(redirectAttribute.toUrl());
		const bool tooManyRedirects = it->redirects >= 5;
		const bool invalidTarget = !nextUrl.isValid() ||
								   (nextUrl.scheme() != QStringLiteral("http") && nextUrl.scheme() != QStringLiteral("https")) ||
								   !nextUrl.userInfo().isEmpty() || nextUrl.host().isEmpty() ||
								   (it->currentUrl.scheme() == QStringLiteral("https") && nextUrl.scheme() == QStringLiteral("http"));
		AReply->deleteLater();
		if (tooManyRedirects || invalidTarget)
		{
			failDownload(AUrlStr, it->currentUrl);
			return;
		}
		++it->redirects;
		it->reply = nullptr;
		it->data.clear();
		it->received = 0;
		startRequestForUrl(AUrlStr, nextUrl);
		return;
	}

	const QNetworkReply::NetworkError networkError = AReply->error();
	const QByteArray tail = readReplyData(AReply);
	if (it->received + tail.size() > it->maxBytes)
		it->tooLarge = true;
	else
	{
		it->received += tail.size();
		it->data.append(tail);
	}
	AReply->deleteLater();

	const InFlightDownload entry = FInFlight.take(AUrlStr);
	FActiveDownloads = qMax(0, FActiveDownloads - 1);
	const bool ok = networkError == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300 &&
					!entry.tooLarge && !entry.data.isEmpty();
	const QByteArray data = ok ? entry.data : QByteArray();
	for (const DataHandler &handler : entry.waiters)
		handler(data);
	startQueuedDownloads();
}

void NetworkManager::startQueuedDownloads()
{
	while (FActiveDownloads < IV_MAX_CONCURRENT_DOWNLOADS && !FQueued.isEmpty())
	{
		QueuedRequest req = FQueued.dequeue();
		if (FInFlight.contains(req.urlStr))
			FInFlight[req.urlStr].waiters.append(req.handler);
		else
			doNetworkRequest(req.urlStr, req.url, req.maxBytes, req.handler);
	}
}
