#include "saslauthfeature.h"

#include <QtEndian>
#include <QMultiHash>
#include <QStringList>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <definitions/namespaces.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

#define AUTH_PLAIN          "PLAIN"
#define AUTH_ANONYMOUS      "ANONYMOUS"
#define AUTH_DIGEST_MD5     "DIGEST-MD5"
#define AUTH_SCRAM_SHA1     "SCRAM-SHA-1"
#define AUTH_SCRAM_SHA224   "SCRAM-SHA-224"
#define AUTH_SCRAM_SHA256   "SCRAM-SHA-256"
#define AUTH_SCRAM_SHA384   "SCRAM-SHA-384"
#define AUTH_SCRAM_SHA512   "SCRAM-SHA-512"
#define SCRAM_SHA1_DKLEN    20
#define SCRAM_SHA224_DKLEN  28
#define SCRAM_SHA256_DKLEN  32
#define SCRAM_SHA384_DKLEN  48
#define SCRAM_SHA512_DKLEN  64

static const QStringList SupportedMechanisms = QStringList() << AUTH_SCRAM_SHA512 << AUTH_SCRAM_SHA384 << AUTH_SCRAM_SHA256 << AUTH_SCRAM_SHA224 << AUTH_SCRAM_SHA1 << AUTH_DIGEST_MD5 << AUTH_PLAIN << AUTH_ANONYMOUS;

static QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm, const QByteArray &password, const QByteArray &salt, int iterations, int dkLen)
{
	if (iterations < 1 || dkLen < 1)
		return QByteArray();

	QByteArray key;
	QByteArray index(4, 0);
	QMessageAuthenticationCode hmac(algorithm, password);
	for (quint32 loop=1; key.length()<dkLen; loop++)
	{
		hmac.reset();
		hmac.addData(salt);
		qToBigEndian(loop, index.data());
		hmac.addData(index);

		QByteArray u = hmac.result();
		QByteArray tkey = u;
		for (int iter=1; iter<iterations; iter++)
		{
			hmac.reset();
			hmac.addData(u);
			u = hmac.result();
			std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(), std::bit_xor<char>());
		}

		key += tkey;
	}
	return key.left(dkLen);
}

static QByteArray getDigestMd5ResponseHash(const QMap<QByteArray, QByteArray> &AResponse, const QString &APassword)
{
	QByteArray secret = QCryptographicHash::hash(AResponse.value("username")+':'+AResponse.value("realm")+':'+APassword.toUtf8(),QCryptographicHash::Md5);
	QByteArray a1Hex = QCryptographicHash::hash(secret+':'+AResponse.value("nonce")+':'+AResponse.value("cnonce"),QCryptographicHash::Md5).toHex();
	QByteArray a2Hex = QCryptographicHash::hash("AUTHENTICATE:"+AResponse.value("digest-uri"),QCryptographicHash::Md5).toHex();
	QByteArray value = QCryptographicHash::hash(a1Hex+':'+AResponse.value("nonce")+':'+AResponse.value("nc")+':'+AResponse.value("cnonce")+':'+AResponse.value("qop")+':'+a2Hex,QCryptographicHash::Md5);
	return value.toHex();
}

static QMap<QByteArray, QByteArray> parseChallenge(const QByteArray &AChallenge)
{
	int startPos = 0;
	bool quoting = false;
	QList<QByteArray> paramsList;
	for (int pos=1; pos<AChallenge.size(); pos++)
	{
		const char c = AChallenge.at(pos);
		if (c == '\"')
		{
			if (quoting && AChallenge.at(pos-1)!='\\')
				quoting = false;
			else if (!quoting)
				quoting = true;
		}
		else if (c==',' && !quoting)
		{
			paramsList.append(AChallenge.mid(startPos,pos-startPos));
			startPos=pos+1;
		}
	}
	paramsList.append(AChallenge.mid(startPos));

	QMap<QByteArray, QByteArray> map;
	for (int i = 0; i<paramsList.count(); i++)
	{
		QByteArray param = paramsList.at(i).trimmed();
		int delimIndex = param.indexOf('=');
		if (delimIndex > 0)
		{
			QByteArray key = param.left(delimIndex);
			QByteArray value = param.right(param.length()-delimIndex-1);
			if (value.startsWith('"') && value.endsWith('"'))
			{
				value.remove(0,1).chop(1);
				value.replace("\\\"", "\"");
				value.replace("\\\\", "\\");
			}
			map.insert(key,value);
		}
	}

	return map;
}

static QByteArray serializeResponse(const QMap<QByteArray, QByteArray> &AResponse)
{
	QByteArray response;
	for (QMap<QByteArray, QByteArray>::const_iterator it=AResponse.constBegin(); it!=AResponse.constEnd(); ++it)
	{
		QByteArray value = it.value();
		value.replace("\\", "\\\\");
		value.replace("\"", "\\\"");
		response.append(it.key() + "=\"" + value + "\",");
	}
	response.chop(1);
	return response;
}

SASLAuthFeature::SASLAuthFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
	connect(FXmppStream->instance(),SIGNAL(passwordProvided(const QString &)),SLOT(onXmppStreamPasswordProvided(const QString &)));
}

SASLAuthFeature::~SASLAuthFeature()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool SASLAuthFeature::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.kind() == "challenge")
		{
			QByteArray challengeData = QByteArray::fromBase64(AStanza.element().text().toLatin1());
			LOG_STRM_DEBUG(FXmppStream->streamJid(),QString("SASL auth challenge received: %1").arg(QString::fromUtf8(challengeData)));

			QByteArray responseData;
			QMap<QByteArray, QByteArray> responseMap;
			QMap<QByteArray, QByteArray> challengeMap = parseChallenge(challengeData);

			if (FSelectedMechanism==AUTH_DIGEST_MD5 && !challengeMap.value("nonce").isEmpty())
			{
				QByteArray randBytes(32,' ');
				for (int i=0; i<randBytes.size(); i++)
					randBytes[i] = static_cast<char>(256.0 * qrand() / (RAND_MAX + 1.0));

				responseMap["cnonce"] = randBytes.toHex();
				if (challengeMap.contains("realm"))
					responseMap["realm"] = challengeMap.value("realm");
				else
					responseMap["realm"] = FXmppStream->streamJid().pDomain().toUtf8();
				responseMap["username"] = FXmppStream->streamJid().pNode().toUtf8();
				responseMap["nonce"] = challengeMap.value("nonce");
				responseMap["nc"] = "00000001";
				responseMap["qop"] = "auth";
				responseMap["digest-uri"] = QString("xmpp/%1").arg(FXmppStream->streamJid().pDomain()).toUtf8();
				responseMap["charset"] = "utf-8";
				responseMap["response"] = getDigestMd5ResponseHash(responseMap,FXmppStream->password());

				responseData = serializeResponse(responseMap);
			}
			else if (FSelectedMechanism.startsWith("SCRAM-SHA") && SupportedMechanisms.contains(FSelectedMechanism) && !challengeMap.value("r").isEmpty())
			{
				QByteArray serverNonce = challengeMap.value("r");
				if (!serverNonce.startsWith(SCRAMSHA_clientNonce))
				{
					XmppError err(IERR_SASL_AUTH_INVALID_REQUEST);
					LOG_STRM_WARNING(FXmppStream->streamJid(), QString("SCRAM-SHA error: Invalid server challenge nonce"));
					emit error(err);
					return true;
				}

				QCryptographicHash::Algorithm method;
				int len = 0;

				if (FSelectedMechanism == AUTH_SCRAM_SHA1)
				{
					method = QCryptographicHash::Sha1;
					len = SCRAM_SHA1_DKLEN;
				}
				else if (FSelectedMechanism == AUTH_SCRAM_SHA224)
				{
					method = QCryptographicHash::Sha224;
					len = SCRAM_SHA224_DKLEN;
				}
				else if (FSelectedMechanism == AUTH_SCRAM_SHA256)
				{
					method = QCryptographicHash::Sha256;
					len = SCRAM_SHA256_DKLEN;
				}
				else if (FSelectedMechanism == AUTH_SCRAM_SHA384)
				{
					method = QCryptographicHash::Sha384;
					len = SCRAM_SHA384_DKLEN;
				}
				else if (FSelectedMechanism == AUTH_SCRAM_SHA512)
				{
					method = QCryptographicHash::Sha512;
					len = SCRAM_SHA512_DKLEN;
				}

				int iterations = challengeMap.value("i").toInt();
				QByteArray salt = QByteArray::fromBase64(challengeMap.value("s"));
				QByteArray saltedPassword = deriveKeyPbkdf2(method, FXmppStream->password().toUtf8(), salt, iterations, len);

				QByteArray clientKey = QMessageAuthenticationCode::hash("Client Key", saltedPassword, method);
				QByteArray serverKey = QMessageAuthenticationCode::hash("Server Key", saltedPassword, method);

				QByteArray serverFirstMessage = challengeData;
				QByteArray clientFinalMessageBare = "c=biws,r=" + serverNonce;
				QByteArray authMessage = SCRAMSHA_initialMessage + "," + serverFirstMessage + "," + clientFinalMessageBare;

				QByteArray storedKey = QCryptographicHash::hash(clientKey, method);
				QByteArray clientSignature = QMessageAuthenticationCode::hash(authMessage, storedKey, method);
				SCRAMSHA_ServerSignature = QMessageAuthenticationCode::hash(authMessage, serverKey, method);

				QByteArray clientProof = clientKey;
				for (int i = 0; i < clientProof.size(); ++i)
					clientProof[i] = clientProof[i] ^ clientSignature[i];

				responseData = clientFinalMessageBare + ",p=" + clientProof.toBase64();
			}

			Stanza response("response",NS_FEATURE_SASL);
			response.element().appendChild(response.createTextNode(responseData.toBase64()));
			FXmppStream->sendStanza(response);

			LOG_STRM_DEBUG(FXmppStream->streamJid(),QString("SASL auth response sent: %1").arg(QString::fromUtf8(responseData)));
		}
		else
		{
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			if (AStanza.kind() == "success")
			{
				if (FSelectedMechanism.startsWith("SCRAM-SHA") && SupportedMechanisms.contains(FSelectedMechanism))
				{
					QByteArray serverSignature = QByteArray::fromBase64(AStanza.element().text().toLatin1());
					QByteArray savedServerSignature = QByteArray("v=").append(SCRAMSHA_ServerSignature.toBase64());
					if (serverSignature != savedServerSignature)
					{
						XmppError err(IERR_SASL_AUTH_INVALID_RESPONSE);
						LOG_STRM_WARNING(FXmppStream->streamJid(),QString("SCRAM-SHA error: Invalid server response signature"));
						emit error(err);
						return true;
					}
				}

				LOG_STRM_INFO(FXmppStream->streamJid(),"Authorization successes");
				deleteLater();
				emit finished(true);
			}
			else if (AStanza.kind() == "failure")
			{
				XmppSaslError err(AStanza.element());
				LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Authorization failed: %1").arg(err.condition()));
				emit error(err);
			}
			else
			{
				XmppError err(IERR_SASL_AUTH_INVALID_RESPONSE);
				LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Authorization error: Invalid stanza kind=%1").arg(AStanza.kind()));
				emit error(err);
			}
		}
		return true;
	}
	return false;
}

bool SASLAuthFeature::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString SASLAuthFeature::featureNS() const
{
	return NS_FEATURE_SASL;
}

IXmppStream *SASLAuthFeature::xmppStream() const
{
	return FXmppStream;
}

bool SASLAuthFeature::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "mechanisms")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			QStringList mechanisms;
			QDomElement mechElem = AElem.firstChildElement("mechanism");
			while (!mechElem.isNull())
			{
				QString mech = mechElem.text().toUpper();
				if (SupportedMechanisms.contains(mech))
					mechanisms.append(mech);
				mechElem = mechElem.nextSiblingElement("mechanism");
			}

			if (!mechanisms.isEmpty())
			{
				if (!FXmppStream->requestPassword())
					sendAuthRequest(mechanisms);
				else
					FMechanisms = mechanisms;
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to send authorization request: Supported mechanism not found"));
			}
		}
		else
		{
			XmppError err(IERR_XMPPSTREAM_NOT_SECURE);
			LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to send authorization request: %1").arg(err.condition()));
			emit error(err);
		}
	}
	else
	{
		LOG_STRM_ERROR(FXmppStream->streamJid(),QString("Failed to send authorization request: Invalid element=%1").arg(AElem.tagName()));
	}

	deleteLater();
	return false;
}

void SASLAuthFeature::authRequestSCRAM(Stanza &AAuth, const QString AMethod)
{
	QByteArray randBytes(32, ' ');
	for (int i = 0; i < randBytes.size(); i++)
		randBytes[i] = static_cast<char>(256.0 * qrand() / (RAND_MAX + 1.0));
	SCRAMSHA_clientNonce = randBytes.toHex();

	QByteArray gs2Header = "n,,"; // TODO: SCRAM-SHA no Channel Binding support yet, base64 "biws"
	SCRAMSHA_initialMessage.append("n=").append(FXmppStream->streamJid().pNode().toUtf8()).append(",r=").append(SCRAMSHA_clientNonce);

	AAuth.setAttribute("mechanism", AMethod);
	AAuth.element().appendChild(AAuth.createTextNode(gs2Header.append(SCRAMSHA_initialMessage).toBase64()));
}

void SASLAuthFeature::sendAuthRequest(const QStringList &AMechanisms)
{
	Stanza auth("auth",NS_FEATURE_SASL);
	if (AMechanisms.contains(AUTH_SCRAM_SHA512))
	{
		authRequestSCRAM(auth, AUTH_SCRAM_SHA512);
		LOG_STRM_INFO(FXmppStream->streamJid(), "SCRAM-SHA-512 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_SCRAM_SHA384))
	{
		authRequestSCRAM(auth, AUTH_SCRAM_SHA384);
		LOG_STRM_INFO(FXmppStream->streamJid(), "SCRAM-SHA-384 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_SCRAM_SHA256))
	{
		authRequestSCRAM(auth, AUTH_SCRAM_SHA256);
		LOG_STRM_INFO(FXmppStream->streamJid(), "SCRAM-SHA-256 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_SCRAM_SHA224))
	{
		authRequestSCRAM(auth, AUTH_SCRAM_SHA224);
		LOG_STRM_INFO(FXmppStream->streamJid(), "SCRAM-SHA-224 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_SCRAM_SHA1))
	{
		authRequestSCRAM(auth, AUTH_SCRAM_SHA1);
		LOG_STRM_INFO(FXmppStream->streamJid(), "SCRAM-SHA-1 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_DIGEST_MD5))
	{
		auth.setAttribute("mechanism",AUTH_DIGEST_MD5);
		LOG_STRM_INFO(FXmppStream->streamJid(),"Digest-MD5 authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_PLAIN))
	{
		QByteArray data;
		data.append('\0').append(FXmppStream->streamJid().pNode().toUtf8()).append('\0').append(FXmppStream->password().toUtf8());

		auth.setAttribute("mechanism",AUTH_PLAIN);
		auth.element().appendChild(auth.createTextNode(data.toBase64()));
		LOG_STRM_INFO(FXmppStream->streamJid(),"Plain authorization request sent");
	}
	else if (AMechanisms.contains(AUTH_ANONYMOUS))
	{
		Stanza auth("auth");
		auth.setAttribute("mechanism",AUTH_ANONYMOUS);
		LOG_STRM_INFO(FXmppStream->streamJid(),"Anonymous authorization request sent");
	}

	FSelectedMechanism = auth.attribute("mechanism");
	FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	FXmppStream->sendStanza(auth);
}

void SASLAuthFeature::onXmppStreamPasswordProvided(const QString &APassword)
{
	Q_UNUSED(APassword);
	if (!FMechanisms.isEmpty())
	{
		sendAuthRequest(FMechanisms);
		FMechanisms.clear();
	}
}
