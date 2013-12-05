#include "saslauth.h"

#include <QMultiHash>
#include <QStringList>
#include <QCryptographicHash>

#define AUTH_PLAIN        "PLAIN"
#define AUTH_ANONYMOUS    "ANONYMOUS"
#define AUTH_DIGEST_MD5   "DIGEST-MD5"

static QMap<QByteArray, QByteArray> parseChallenge(const QByteArray &AChallenge)
{
	int startPos = 0;
	bool quoting = false;
	QList<QByteArray> paramsList;
	for (int pos = 1; pos<AChallenge.size(); pos++)
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
	foreach (const QByteArray &key, AResponse.keys())
	{
		QByteArray value = AResponse[key];
		value.replace("\\", "\\\\");
		value.replace("\"", "\\\"");
		response.append(key + "=\"" + value + "\",");
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

SASLAuth::SASLAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FChallengeStep = 0;
	FXmppStream = AXmppStream;
}

SASLAuth::~SASLAuth()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool SASLAuth::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.tagName() == "challenge")
		{
			if (FChallengeStep == 0)
			{
				FChallengeStep++;
				QMap<QByteArray, QByteArray> challengeMap = parseChallenge(QByteArray::fromBase64(AStanza.element().text().toLatin1()));

				QMap<QByteArray, QByteArray> responseMap;
				QByteArray randBytes(32,' ');
				for (int i=0; i<randBytes.size(); i++)
					randBytes[i] = (char) (256.0 * qrand() / (RAND_MAX + 1.0));
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
				responseMap["response"] = getResponseValue(responseMap,FXmppStream->getSessionPassword());

				Stanza response("response");
				response.setAttribute("xmlns",NS_FEATURE_SASL);
				response.element().appendChild(response.createTextNode(serializeResponse(responseMap).toBase64()));
				FXmppStream->sendStanza(response);
			}
			else if (FChallengeStep == 1)
			{
				FChallengeStep--;
				Stanza response("response");
				response.setAttribute("xmlns",NS_FEATURE_SASL);
				FXmppStream->sendStanza(response);
			}
		}
		else
		{
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			if (AStanza.tagName() == "success")
			{
				deleteLater();
				emit finished(true);
			}
			else if (AStanza.tagName() == "failure")
			{
				emit error(XmppStanzaError(AStanza.element()));
			}
			else if (AStanza.tagName() == "abort")
			{
				XmppStanzaError err(XmppStanzaError::EC_NOT_AUTHORIZED);
				err.setAppCondition(NS_FEATURE_SASL,XERR_SASL_ABORTED);
				emit error(err);
			}
			else
			{
				emit error(XmppError(IERR_SASL_AUTH_INVALID_RESPONCE));
			}
		}
		return true;
	}
	return false;
}

bool SASLAuth::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

QString SASLAuth::featureNS() const
{
	return NS_FEATURE_SASL;
}

IXmppStream *SASLAuth::xmppStream() const
{
	return FXmppStream;
}

bool SASLAuth::start(const QDomElement &AElem)
{
	if (AElem.tagName()=="mechanisms")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			FChallengeStep = 0;

			QList<QString> mechList;
			QDomElement mechElem = AElem.firstChildElement("mechanism");
			while (!mechElem.isNull())
			{
				mechList.append(mechElem.text());
				mechElem = mechElem.nextSiblingElement("mechanism");
			}

			if (mechList.contains(AUTH_DIGEST_MD5))
			{
				Stanza auth("auth");
				auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_DIGEST_MD5);
				FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
				FXmppStream->sendStanza(auth);
				return true;
			}
			else if (mechList.contains(AUTH_PLAIN))
			{
				QByteArray resp;
				resp.append('\0').append(FXmppStream->streamJid().pNode().toUtf8()).append('\0').append(FXmppStream->getSessionPassword().toUtf8());

				Stanza auth("auth");
				auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_PLAIN);
				auth.element().appendChild(auth.createTextNode(resp.toBase64()));
				FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
				FXmppStream->sendStanza(auth);
				return true;
			}
			else if (mechList.contains(AUTH_ANONYMOUS))
			{
				Stanza auth("auth");
				auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_ANONYMOUS);
				FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
				FXmppStream->sendStanza(auth);
				return true;
			}
		}
		else
		{
			emit error(XmppError(IERR_XMPPSTREAM_NOT_SECURE));
		}
	}
	deleteLater();
	return false;
}
