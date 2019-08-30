#include "saslauthfeature.h"

#include <QMultiHash>
#include <QStringList>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QtCore/QtEndian>
#include <definitions/namespaces.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

#define AUTH_PLAIN        "PLAIN"
#define AUTH_ANONYMOUS    "ANONYMOUS"
#define AUTH_DIGEST_MD5   "DIGEST-MD5"
#define AUTH_SCRAM_SHA1   "SCRAM-SHA-1"

static const QStringList SupportedMechanisms = QStringList() << AUTH_SCRAM_SHA1 << AUTH_DIGEST_MD5 << AUTH_PLAIN << AUTH_ANONYMOUS;

static QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm, const QByteArray &password, const QByteArray &salt, int iterations, quint64 dkLen)
{
   if (iterations < 1 || dkLen < 1)
	   return QByteArray();

   QByteArray key;
   quint32 currentIteration = 1;
   QMessageAuthenticationCode hmac(algorithm, password);
   QByteArray index(4, Qt::Uninitialized);
   while (quint64(key.length()) < dkLen)
   {
	   hmac.addData(salt);
	   qToBigEndian(currentIteration, index.data());
	   hmac.addData(index);

	   QByteArray u = hmac.result();
	   hmac.reset();
	   QByteArray tkey = u;
	   for (int iter = 1; iter < iterations; iter++)
	   {
		   hmac.addData(u);
		   u = hmac.result();
		   hmac.reset();
		   std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(), std::bit_xor<char>());
	   }
	   key += tkey;
	   currentIteration++;
   }
   return key.left(static_cast<int>(dkLen));
}

static QMap<QByteArray, QByteArray> parseChallenge(const QByteArray &AChallenge)
{
	QMap<QByteArray, QByteArray> map;

	foreach (QByteArray line, AChallenge.split(',')) {
		int pos = line.indexOf('=');
		QByteArray key = line.left(pos);
		QByteArray value = line.right(line.size() - pos - 1);
		if (value.startsWith('"') && value.endsWith('"')){
			value.remove(0,1).chop(1);
		}

		map.insert(key,value);
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

static QByteArray getResponseValue(const QMap<QByteArray, QByteArray> &AResponse, const QString &APassword)
{
	QByteArray secret = QCryptographicHash::hash(AResponse.value("username")+':'+AResponse.value("realm")+':'+APassword.toUtf8(),QCryptographicHash::Md5);
	QByteArray a1Hex = QCryptographicHash::hash(secret+':'+AResponse.value("nonce")+':'+AResponse.value("cnonce"),QCryptographicHash::Md5).toHex();
	QByteArray a2Hex = QCryptographicHash::hash("AUTHENTICATE:"+AResponse.value("digest-uri"),QCryptographicHash::Md5).toHex();
	QByteArray value = QCryptographicHash::hash(a1Hex+':'+AResponse.value("nonce")+':'+AResponse.value("nc")+':'+AResponse.value("cnonce")+':'+AResponse.value("qop")+':'+a2Hex,QCryptographicHash::Md5);
	return value.toHex();
}

SASLAuthFeature::SASLAuthFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
	FSelectedMechanism = nullptr;
	SCRAMSHA1_clientNonce = nullptr;
	SCRAMSHA1_initialMessage = nullptr;
	SCRAMSHA1_GS2Header = nullptr;
	SCRAMSHA1_ServerSignature = nullptr;
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
			
			QMap<QByteArray, QByteArray> challengeMap = parseChallenge(challengeData);
			QMap<QByteArray, QByteArray> responseMap;
			QByteArray responseData;

			if (!challengeMap.value("nonce").isEmpty() && FSelectedMechanism == AUTH_DIGEST_MD5)
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
				responseMap["response"] = getResponseValue(responseMap,FXmppStream->password());

				responseData = serializeResponse(responseMap);
			}
			else if (!challengeMap.value("r").isEmpty() && FSelectedMechanism == AUTH_SCRAM_SHA1)
			{
				QByteArray serverFirstMessage = challengeData;
				int iterations = challengeMap.value("i").toInt();
				QByteArray salt = QByteArray::fromBase64(challengeMap.value("s"));
				QByteArray serverNonce = challengeMap.value("r");
				if (!serverNonce.startsWith(SCRAMSHA1_clientNonce))
					goto error_jmp;

				QByteArray clientFinalMessageBare = "c=biws,r=" + serverNonce;
				// Sha1 dkLen always 20
				QByteArray saltedPassword = deriveKeyPbkdf2(QCryptographicHash::Sha1, FXmppStream->password().toUtf8(), salt, iterations, 20);
				QByteArray clientKey = QMessageAuthenticationCode::hash("Client Key", saltedPassword, QCryptographicHash::Sha1);
				QByteArray storedKey = QCryptographicHash::hash(clientKey, QCryptographicHash::Sha1);
				QByteArray authMessage = SCRAMSHA1_initialMessage + "," + serverFirstMessage + "," + clientFinalMessageBare;
				QByteArray clientSignature = QMessageAuthenticationCode::hash(authMessage, storedKey, QCryptographicHash::Sha1);
				QByteArray clientProof = clientKey;
				for (int i = 0; i < clientProof.size(); ++i)
				{
					clientProof[i] = clientProof[i] ^ clientSignature[i];
				}
				QByteArray serverKey = QMessageAuthenticationCode::hash("Server Key", saltedPassword, QCryptographicHash::Sha1);
				SCRAMSHA1_ServerSignature = QMessageAuthenticationCode::hash(authMessage, serverKey, QCryptographicHash::Sha1);
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
				if (FSelectedMechanism == AUTH_SCRAM_SHA1)
				{
					QByteArray verificationServerSignature = QByteArray::fromBase64(AStanza.element().text().toLatin1());
					QByteArray savedServerSignature = QByteArray("v=").append(SCRAMSHA1_ServerSignature.toBase64());
					if (verificationServerSignature != savedServerSignature)
						goto error_jmp;
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
				error_jmp:
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

void SASLAuthFeature::sendAuthRequest(const QStringList &AMechanisms)
{
	Stanza auth("auth",NS_FEATURE_SASL);
	if (AMechanisms.contains(AUTH_SCRAM_SHA1))
	{
		QByteArray randBytes(32,' ');
		for (int i=0; i<randBytes.size(); i++)
			randBytes[i] = static_cast<char>(256.0 * qrand() / (RAND_MAX + 1.0));

		SCRAMSHA1_clientNonce = randBytes.toHex();
		SCRAMSHA1_GS2Header = "n,,"; // TODO: SCRAM-SHA1 no Channel Binding support yet, base64 "biws"
		SCRAMSHA1_initialMessage.append("n=").append(FXmppStream->streamJid().pNode().toUtf8()).append(",r=").append(SCRAMSHA1_clientNonce);

		auth.setAttribute("mechanism",AUTH_SCRAM_SHA1);
		auth.element().appendChild(auth.createTextNode(SCRAMSHA1_GS2Header.append(SCRAMSHA1_initialMessage).toBase64()));
		LOG_STRM_INFO(FXmppStream->streamJid(),"SCRAM-SHA1 authorization request sent");
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
