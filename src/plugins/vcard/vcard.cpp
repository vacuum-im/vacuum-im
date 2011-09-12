#include "vcard.h"
#include "vcardplugin.h"

#include <QFile>
#include <QBuffer>
#include <QImageReader>

#define DEFAUL_IMAGE_FORMAT       "png"

VCard::VCard(const Jid &AContactJid, VCardPlugin *APlugin) : QObject(APlugin)
{
	FContactJid = AContactJid;
	FVCardPlugin = APlugin;
	connect(FVCardPlugin,SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
	connect(FVCardPlugin,SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardPublished(const Jid &)));
	connect(FVCardPlugin,SIGNAL(vcardError(const Jid &, const QString &)),
	        SLOT(onVCardError(const Jid &, const QString &)));
	loadVCardFile();
}

VCard::~VCard()
{

}

QString VCard::value(const QString &AName, const QStringList &ATags, const QStringList &ATagList) const
{
	bool tagsFaild = true;
	QDomElement elem = firstElementByName(AName);
	while (!elem.isNull() && tagsFaild)
	{
		tagsFaild = false;
		QDomElement parentElem = elem.parentNode().toElement();
		foreach(QString tag, ATagList)
		{
			QDomElement tagElem = parentElem.firstChildElement(tag);
			if ((tagElem.isNull() && ATags.contains(tag)) || (!tagElem.isNull() && !ATags.contains(tag)))
			{
				tagsFaild = true;
				elem = nextElementByName(AName,elem);
				break;
			}
		}
	}
	return elem.text();
}

QMultiHash<QString,QStringList> VCard::values(const QString &AName, const QStringList &ATagList) const
{
	QMultiHash<QString,QStringList> result;
	QDomElement elem = firstElementByName(AName);
	while (!elem.isNull())
	{
		if (!elem.text().isEmpty())
		{
			QStringList tags;
			QDomElement parentElem = elem.parentNode().toElement();
			foreach(QString tag, ATagList)
				if (!parentElem.firstChildElement(tag).isNull())
					tags.append(tag);
			result.insertMulti(elem.text(),tags);
		}
		elem = nextElementByName(AName, elem);
	}
	return result;
}

void VCard::setTagsForValue(const QString &AName, const QString &AValue, const QStringList &ATags,
                            const QStringList &ATagList)
{
	QDomElement elem = firstElementByName(AName);
	while (!elem.isNull() && elem.text()!=AValue)
		elem = nextElementByName(AName,elem);

	if (elem.isNull())
	{
		elem = createElementByName(AName,ATags,ATagList);
		setTextToElem(elem,AValue);
	}

	if (!ATags.isEmpty() || !ATagList.isEmpty())
	{
		elem = elem.parentNode().toElement();
		foreach(QString tag, ATags)
			if (elem.firstChildElement(tag).isNull())
				elem.appendChild(FDoc.createElement(tag));

		elem = elem.firstChildElement();
		while (!elem.isNull())
		{
			QDomElement nextElem = elem.nextSiblingElement();
			if (ATagList.contains(elem.tagName()) && !ATags.contains(elem.tagName()))
				elem.parentNode().removeChild(elem);
			elem = nextElem;
		}
	}
}

void VCard::setValueForTags(const QString &AName, const QString &AValue, const QStringList &ATags,
                            const QStringList &ATagList)
{
	bool tagsFaild = true;
	QDomElement elem = firstElementByName(AName);
	while (!elem.isNull() && tagsFaild)
	{
		tagsFaild = false;
		QDomElement parentElem = elem.parentNode().toElement();
		foreach(QString tag, ATagList)
		{
			QDomElement tagElem = parentElem.firstChildElement(tag);
			if ((tagElem.isNull() && ATags.contains(tag)) || (!tagElem.isNull() && !ATags.contains(tag)))
			{
				tagsFaild = true;
				elem = nextElementByName(AName,elem);
				break;
			}
		}
	}

	if (elem.isNull())
		elem = createElementByName(AName,ATags,ATagList);
	setTextToElem(elem,AValue);

	if (!ATags.isEmpty())
	{
		elem = elem.parentNode().toElement();
		foreach(QString tag, ATags)
			if (elem.firstChildElement(tag).isNull())
				elem.appendChild(FDoc.createElement(tag));
	}
}

void VCard::setLogoImage(const QImage &AImage, const QByteArray &AFormat)
{
	if (!AImage.isNull())
	{
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		QByteArray format = checkImageFormat(AFormat);
		AImage.save(&buffer,format);
		setValueForTags(VVN_LOGO_TYPE,formatToType(format));
		setValueForTags(VVN_LOGO_VALUE,bytes.toBase64());
	}
	else
	{
		setValueForTags(VVN_LOGO_TYPE,QString::null);
		setValueForTags(VVN_LOGO_VALUE,QString::null);
	}
	FLogo = AImage;
}

void VCard::setPhotoImage(const QImage &AImage, const QByteArray &AFormat)
{
	if (!AImage.isNull())
	{
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		QByteArray format = checkImageFormat(AFormat);
		AImage.save(&buffer,format);
		setValueForTags(VVN_PHOTO_TYPE,formatToType(format));
		setValueForTags(VVN_PHOTO_VALUE,bytes.toBase64());
	}
	else
	{
		setValueForTags(VVN_PHOTO_TYPE,QString::null);
		setValueForTags(VVN_PHOTO_VALUE,QString::null);
	}
	FPhoto = AImage;
}

void VCard::clear()
{
	FDoc.documentElement().removeChild(FDoc.documentElement().firstChildElement(VCARD_TAGNAME));
	FDoc.documentElement().appendChild(FDoc.createElementNS(NS_VCARD_TEMP,VCARD_TAGNAME));
	FPhoto = QImage();
	FLogo = QImage();
}

bool VCard::update(const Jid &AStreamJid)
{
	if (FContactJid.isValid())
		return FVCardPlugin->requestVCard(AStreamJid,FContactJid);
	return false;
}

bool VCard::publish(const Jid &AStreamJid)
{
	if (isValid() && AStreamJid.isValid())
		return FVCardPlugin->publishVCard(this,AStreamJid);
	return false;
}

void VCard::unlock()
{
	FVCardPlugin->unlockVCard(FContactJid);
}

void VCard::loadVCardFile()
{
	QFile vcardFile(FVCardPlugin->vcardFileName(FContactJid));
	if (vcardFile.exists() && vcardFile.open(QIODevice::ReadOnly))
	{
		FDoc.setContent(vcardFile.readAll());
		vcardFile.close();
	}
	if (vcardElem().isNull())
	{
		FDoc.clear();
		QDomElement elem = FDoc.appendChild(FDoc.createElement(VCARD_FILE_ROOT_TAGNAME)).toElement();
		elem.setAttribute("jid",FContactJid.full());
		elem.appendChild(FDoc.createElementNS(NS_VCARD_TEMP,VCARD_TAGNAME));
	}
	else
	{
		FLoadDateTime = QDateTime::fromString(FDoc.documentElement().attribute("dateTime"),Qt::ISODate);
	}

	QByteArray imageData = QByteArray::fromBase64(value(VVN_LOGO_VALUE).toAscii());
	FLogo = !imageData.isEmpty() ? QImage::fromData(imageData) : QImage();

	imageData = QByteArray::fromBase64(value(VVN_PHOTO_VALUE).toAscii());
	FPhoto = !imageData.isEmpty() ? QImage::fromData(imageData) : QImage();

	emit vcardUpdated();
}

QByteArray VCard::checkImageFormat(const QByteArray &AFormat) const
{
	if (QImageReader::supportedImageFormats().contains(AFormat.toLower()))
		return AFormat.toLower();
	return DEFAUL_IMAGE_FORMAT;
}

QString VCard::formatToType(const QByteArray &AFormat) const
{
	if (!AFormat.isEmpty())
		return QString("image/%1").arg(AFormat.toLower().data());
	return QString::null;
}

QDomElement VCard::createElementByName(const QString AName, const QStringList &ATags,
                                       const QStringList &ATagList)
{
	QStringList tagTree = AName.split('/',QString::SkipEmptyParts);
	QDomElement elem = vcardElem().firstChildElement(tagTree.at(0));

	bool tagsFaild = !ATags.isEmpty() || !ATagList.isEmpty();
	while (!elem.isNull() && tagsFaild)
	{
		tagsFaild = false;
		foreach(QString tag, ATagList)
		{
			QDomElement tagElem = elem.firstChildElement(tag);
			if ((tagElem.isNull() && ATags.contains(tag)) || (!tagElem.isNull() && !ATags.contains(tag)))
			{
				tagsFaild = true;
				elem = elem.nextSiblingElement(elem.tagName());
				break;
			}
		}
	}

	if (elem.isNull())
		elem = vcardElem().appendChild(FDoc.createElement(tagTree.at(0))).toElement();

	for (int deep = 1; deep<tagTree.count(); deep++)
		elem = elem.appendChild(FDoc.createElement(tagTree.at(deep))).toElement();

	return elem;
}

QDomElement VCard::firstElementByName(const QString AName) const
{
	int index = 0;
	QDomElement elem = vcardElem();
	QStringList tagTree = AName.split('/',QString::SkipEmptyParts);
	while (!elem.isNull() && index < tagTree.count())
		elem = elem.firstChildElement(tagTree.at(index++));
	return elem;
}

QDomElement VCard::nextElementByName(const QString AName, const QDomElement APrevElem) const
{
	QDomElement elem = APrevElem;
	QStringList tagTree = AName.split('/',QString::SkipEmptyParts);
	int index = tagTree.count();
	while (index > 1)
	{
		index--;
		elem = elem.parentNode().toElement();
	}
	elem = elem.nextSiblingElement(elem.tagName());
	while (!elem.isNull() && index < tagTree.count())
		elem = elem.firstChildElement(tagTree.at(index++));
	return elem;
}

QDomElement VCard::setTextToElem(QDomElement &AElem, const QString &AText) const
{
	if (!AElem.isNull())
	{
		QDomNode node = AElem.firstChild();
		while (!node.isNull() && !node.isText())
			node = node.nextSibling();
		if (node.isNull() && !AText.isEmpty())
			AElem.appendChild(AElem.ownerDocument().createTextNode(AText));
		else if (!node.isNull() && !AText.isNull())
			node.toText().setData(AText);
		else if (!node.isNull())
			AElem.removeChild(node);
	}
	return AElem;
}

void VCard::onVCardReceived(const Jid &AContactJid)
{
	if (FContactJid == AContactJid)
		loadVCardFile();
}

void VCard::onVCardPublished(const Jid &AContactJid)
{
	if (FContactJid && AContactJid)
		emit vcardPublished();
}

void VCard::onVCardError(const Jid &AContactJid, const QString &AError)
{
	if (FContactJid == AContactJid)
		emit vcardError(AError);
}

