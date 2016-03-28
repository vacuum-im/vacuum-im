#include "avatars.h"

#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QFileDialog>
#include <QImageReader>
#include <QCryptographicHash>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/vcardvaluenames.h>
#include <utils/pluginhelper.h>
#include <utils/imagemanager.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

#define DIR_AVATARS               "avatars"

#define SHC_PRESENCE              "/presence"
#define SHC_IQ_AVATAR             "/iq[@type='get']/query[@xmlns='" NS_JABBER_IQ_AVATAR "']"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

#define AVATAR_IQ_TIMEOUT         30000

#define EMPTY_AVATAR_HASH         QString("")
#define UNKNOWN_AVATAR_HASH       QString::null

static const QList<int> AvatarRosterKinds = QList<int>() << RIK_STREAM_ROOT << RIK_CONTACT;

void NormalizeAvatarImage(const QImage &AImage, quint8 ASize, QImage &AColor, QImage &AGray)
{
	AColor = ASize>0 ? ImageManager::squared(AImage, ASize) : AImage;
	AGray = ImageManager::opacitized(ImageManager::grayscaled(AColor));
}

LoadAvatarTask::LoadAvatarTask(QObject *AAvatars, const QString &AFile, quint8 ASize, bool AVCard) : QRunnable()
{
	FFile = AFile;
	FSize = ASize;
	FVCard = AVCard;
	FAvatars = AAvatars;
	setAutoDelete(false);

	FHash = EMPTY_AVATAR_HASH;
}

QByteArray LoadAvatarTask::parseFile(QFile *AFile) const
{
	if (FVCard)
	{
		QString xmlError;
		QDomDocument doc;
		if (doc.setContent(AFile,true,&xmlError))
		{
			QDomElement photoDataElem = doc.documentElement().firstChildElement("vCard").firstChildElement("PHOTO").firstChildElement("BINVAL");
			if (!photoDataElem.isNull())
				return QByteArray::fromBase64(photoDataElem.text().toLatin1());

			QDomElement logoDataElem = doc.documentElement().firstChildElement("vCard").firstChildElement("LOGO").firstChildElement("BINVAL");
			if (!logoDataElem.isNull())
				return QByteArray::fromBase64(logoDataElem.text().toLatin1());
		}
		else
		{
			Logger::reportError(QString("LoadAvatarTask"),QString("Failed to load avatar from vCard file content: %1").arg(xmlError),false);
			AFile->remove();
		}
	}
	else
	{
		return AFile->readAll();
	}
	return QByteArray();
}

void LoadAvatarTask::run()
{
	QFile file(FFile);
	if (file.open(QFile::ReadOnly))
	{
		FData = parseFile(&file);
		if (!FData.isEmpty())
		{
			QImage image = QImage::fromData(FData);
			if (!image.isNull())
			{
				FHash = QString::fromLatin1(QCryptographicHash::hash(FData,QCryptographicHash::Sha1).toHex());
				NormalizeAvatarImage(image,FSize,FColorImage,FGrayImage);
			}
			else
			{
				Logger::reportError(QString("LoadAvatarTask"),QString("Failed to load avatar from data: Unsupported format"),false);
			}
		}
	}
	else if (file.exists())
	{
		Logger::reportError(QString("LoadAvatarTask"),QString("Failed to load avatar from file: %1").arg(file.errorString()),false);
	}

	QMetaObject::invokeMethod(FAvatars,"onLoadAvatarTaskFinished",Qt::QueuedConnection,Q_ARG(LoadAvatarTask *,this));
}


Avatars::Avatars()
{
	FVCardManager = NULL;
	FRostersModel = NULL;
	FStanzaProcessor = NULL;
	FPresenceManager = NULL;
	FXmppStreamManager = NULL;
	FRostersViewPlugin = NULL;

	FAvatarSize = 32;
	FAvatarsVisible = false;
	FAvatarLabelId = AdvancedDelegateItem::NullId;

	qRegisterMetaType<LoadAvatarTask *>("LoadAvatarTask *");
}

Avatars::~Avatars()
{
	FThreadPool.waitForDone();
}

void Avatars::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Avatars");
	APluginInfo->description = tr("Allows to set and display avatars");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(VCARD_UUID);
}

bool Avatars::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardManager").value(0,NULL);
	if (plugin)
	{
		FVCardManager = qobject_cast<IVCardManager *>(plugin->instance());
		if (FVCardManager)
		{
			connect(FVCardManager->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
			connect(FVCardManager->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardPublished(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0,NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)), SLOT(onRosterIndexInserted(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FVCardManager!=NULL;
}

bool Avatars::initObjects()
{
	FAvatarsDir.setPath(PluginHelper::pluginManager()->homePath());
	if (!FAvatarsDir.exists(DIR_AVATARS))
		FAvatarsDir.mkdir(DIR_AVATARS);
	FAvatarsDir.cd(DIR_AVATARS);

	if (FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_AVATARS,this);
	}

	if (FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_AVATAR_IMAGE);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = RDR_AVATAR_IMAGE;
		FAvatarLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
		
		FRostersViewPlugin->rostersView()->insertLabelHolder(RLHO_AVATARS,this);
	}

	onIconStorageChanged();
	connect(IconStorage::staticStorage(RSR_STORAGE_MENUICONS), SIGNAL(storageChanged()), SLOT(onIconStorageChanged()));

	return true;
}

bool Avatars::initSettings()
{
	Options::setDefaultValue(OPV_AVATARS_SMALLSIZE, 24);
	Options::setDefaultValue(OPV_AVATARS_NORMALSIZE, 32);
	Options::setDefaultValue(OPV_AVATARS_LARGESIZE, 64);
	return true;
}

bool Avatars::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIPresenceOut.value(AStreamJid) == AHandlerId)
	{
		QString hash = FStreamAvatars.value(AStreamJid);
		QDomElement vcardUpdate = AStanza.addElement("x",NS_VCARD_UPDATE);

		// vCard-based avatars
		if (hash!=UNKNOWN_AVATAR_HASH && !FBlockingResources.contains(AStreamJid))
		{
			QDomElement photoElem = vcardUpdate.appendChild(AStanza.createElement("photo")).toElement();
			photoElem.appendChild(AStanza.createTextNode(hash));
		}

		// IQ-based avatars
		if (!hash.isEmpty())
		{
			QDomElement iqUpdate = AStanza.addElement("x",NS_JABBER_X_AVATAR);
			QDomElement hashElem = iqUpdate.appendChild(AStanza.createElement("hash")).toElement();
			hashElem.appendChild(AStanza.createTextNode(hash));
		}
	}
	else if (FSHIPresenceIn.value(AStreamJid) == AHandlerId)
	{
		static const QList<QString> acceptStanzaTypes = QList<QString>() << QString(PRESENCE_TYPE_AVAILABLE) << QString(PRESENCE_TYPE_UNAVAILABLE);

		Jid contactJid = AStanza.from();
		if (!FStreamAvatars.contains(contactJid) && acceptStanzaTypes.contains(AStanza.type()))
		{
			QDomElement vcardUpdate = AStanza.firstElement("x",NS_VCARD_UPDATE);
			QDomElement iqUpdate = AStanza.firstElement("x",NS_JABBER_X_AVATAR);
			
			bool isMucPresence = !AStanza.firstElement("x",NS_MUC_USER).isNull();
			Jid vcardJid = isMucPresence ? contactJid : contactJid.bare();

			if (!vcardUpdate.isNull())
			{
				QDomElement photoElem = vcardUpdate.firstChildElement("photo");
				if (!photoElem.isNull())
				{
					QString hash = photoElem.text().toLower();
					if (!updateVCardAvatar(vcardJid,hash,false))
					{
						LOG_STRM_INFO(AStreamJid,QString("Requesting avatar form vCard, jid=%1").arg(vcardJid.full()));
						FVCardManager->requestVCard(AStreamJid,vcardJid);
					}
				}
			}
			else if (AStreamJid.pBare() == contactJid.pBare())
			{
				if (AStanza.type()==PRESENCE_TYPE_AVAILABLE && !FBlockingResources.contains(AStreamJid,contactJid))
				{
					LOG_STRM_INFO(AStreamJid,QString("Resource %1 is now blocking avatar update notify mechanism").arg(contactJid.resource()));
					FBlockingResources.insertMulti(AStreamJid,contactJid);
					if (FStreamAvatars.value(AStreamJid) != UNKNOWN_AVATAR_HASH)
					{
						FStreamAvatars[AStreamJid] = UNKNOWN_AVATAR_HASH;
						updatePresence(AStreamJid);
					}
				}
				else if (AStanza.type()==PRESENCE_TYPE_UNAVAILABLE && FBlockingResources.contains(AStreamJid,contactJid))
				{
					LOG_STRM_INFO(AStreamJid,QString("Resource %1 is stopped blocking avatar update notify mechanism").arg(contactJid.resource()));
					FBlockingResources.remove(AStreamJid,contactJid);
					if (!FBlockingResources.contains(AStreamJid))
						FVCardManager->requestVCard(AStreamJid,AStreamJid.bare());
				}
			}
			else if (!iqUpdate.isNull())
			{
				QString hash = iqUpdate.firstChildElement("hash").text().toLower();
				if (!updateIqAvatar(contactJid,hash))
				{
					Stanza query(STANZA_KIND_IQ);
					query.setType(STANZA_TYPE_GET).setTo(contactJid.full()).setUniqueId();
					query.addElement("query",NS_JABBER_IQ_AVATAR);
					if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,query,AVATAR_IQ_TIMEOUT))
					{
						LOG_STRM_INFO(AStreamJid,QString("Load iq avatar request sent, jid=%1").arg(contactJid.full()));
						FIqAvatarRequests.insert(query.id(),contactJid);
					}
					else
					{
						LOG_STRM_WARNING(AStreamJid,QString("Failed to send load iq avatar request, jid=%1").arg(contactJid.full()));
					}
				}
			}
			else
			{
				if (!FVCardAvatars.contains(vcardJid))
					startLoadVCardAvatar(vcardJid);
				updateIqAvatar(contactJid,UNKNOWN_AVATAR_HASH);
			}
		}
	}
	else if (FSHIIqAvatarIn.value(AStreamJid) == AHandlerId)
	{
		AAccept = true;
		QString fileName = avatarFileName(FStreamAvatars.value(AStreamJid));
		if (!fileName.isEmpty())
		{
			QFile file(fileName);
			if (file.open(QFile::ReadOnly))
			{
				LOG_STRM_INFO(AStreamJid,QString("Sending self avatar to contact, jid=%1").arg(AStanza.from()));
				Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
				QDomElement dataElem = result.addElement("query",NS_JABBER_IQ_AVATAR).appendChild(result.createElement("data")).toElement();
				dataElem.appendChild(result.createTextNode(file.readAll().toBase64()));
				FStanzaProcessor->sendStanzaOut(AStreamJid,result);
			}
			else
			{
				REPORT_ERROR(QString("Failed to load self avatar from file: %1").arg(file.errorString()));
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_INTERNAL_SERVER_ERROR);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Rejected iq avatar request from contact, jid=%1: Avatar is empty").arg(AStanza.from()));
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_ITEM_NOT_FOUND);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
	}
	return false;
}

void Avatars::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FIqAvatarRequests.contains(AStanza.id()))
	{
		Jid contactJid = FIqAvatarRequests.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Received iq avatar from contact, jid=%1").arg(AStanza.from()));
			QDomElement dataElem = AStanza.firstElement("query",NS_JABBER_IQ_AVATAR).firstChildElement("data");
			QByteArray avatarData = QByteArray::fromBase64(dataElem.text().toLatin1());
			updateIqAvatar(contactJid,saveAvatarData(avatarData));
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive iq avatar from contact, jid=%1: %2").arg(AStanza.from(),XmppStanzaError(AStanza).condition()));
			updateIqAvatar(contactJid,UNKNOWN_AVATAR_HASH);
		}
	}
}

QList<int> Avatars::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_AVATARS)
		return QList<int>() << RDR_AVATAR_IMAGE;
	return QList<int>();
}

QVariant Avatars::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder==RDHO_AVATARS && AvatarRosterKinds.contains(AIndex->kind()))
	{
		switch (ARole)
		{
			case RDR_AVATAR_IMAGE:
			{
				bool gray = AIndex->data(RDR_SHOW).toInt()==IPresence::Offline || AIndex->data(RDR_SHOW).toInt()==IPresence::Error;
				return visibleAvatarImage(avatarHash(AIndex->data(RDR_FULL_JID).toString()),FAvatarSize,gray);
			}
		}
	}
	return QVariant();
}


bool Avatars::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
	return false;
}

QList<quint32> Avatars::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AIndex);
	QList<quint32> labels;
	if (AOrder==RLHO_AVATARS && FAvatarsVisible)
		labels.append(FAvatarLabelId);
	return labels;
}

AdvancedDelegateItem Avatars::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AIndex);
	if (AOrder==RLHO_AVATARS && ALabelId==FAvatarLabelId)
		return FRostersViewPlugin->rostersView()->registeredLabel(FAvatarLabelId);
	return AdvancedDelegateItem();
}

quint8 Avatars::avatarSize(int AType) const
{
	switch (AType)
	{
	case AvatarSmall:
		return Options::node(OPV_AVATARS_SMALLSIZE).value().toInt();
	case AvatarLarge:
		return Options::node(OPV_AVATARS_LARGESIZE).value().toInt();
	default:
		return Options::node(OPV_AVATARS_NORMALSIZE).value().toInt();
	}
}

bool Avatars::hasAvatar(const QString &AHash) const
{
	return !AHash.isEmpty() ? QFile::exists(avatarFileName(AHash)) : false;
}

QString Avatars::avatarFileName(const QString &AHash) const
{
	return !AHash.isEmpty() ? FAvatarsDir.filePath(AHash.toLower()) : QString::null;
}

QString Avatars::avatarHash(const Jid &AContactJid, bool AExact) const
{
	QString hash = FCustomPictures.value(AContactJid);
	if (hash == UNKNOWN_AVATAR_HASH)
		hash = FVCardAvatars.value(AContactJid);
	if (hash == UNKNOWN_AVATAR_HASH)
		hash = FIqAvatars.value(AContactJid);
	if (hash==UNKNOWN_AVATAR_HASH && !AExact && AContactJid.hasResource())
		return avatarHash(AContactJid.bare(),true);
	return hash;
}

QString Avatars::saveAvatarData(const QByteArray &AData) const
{
	if (!AData.isEmpty())
	{
		QString hash = QCryptographicHash::hash(AData,QCryptographicHash::Sha1).toHex();
		if (!hasAvatar(hash))
		{
			QImage image = QImage::fromData(AData);
			if (image.isNull())
				LOG_WARNING(QString("Failed to save avatar data, hash=%1: Unsupported format").arg(hash));
			else if (saveFileData(avatarFileName(hash),AData))
				return hash;
		}
		else
		{
			return hash;
		}
	}
	return EMPTY_AVATAR_HASH;
}

QByteArray Avatars::loadAvatarData(const QString &AHash) const
{
	return loadFileData(avatarFileName(AHash));
}

bool Avatars::setAvatar(const Jid &AStreamJid, const QByteArray &AData)
{
	bool published = false;
	QString format = getImageFormat(AData);
	if (AData.isEmpty() || !format.isEmpty())
	{
		IVCard *vcard = FVCardManager!=NULL ? FVCardManager->getVCard(AStreamJid.bare()) : NULL;
		if (vcard)
		{
			if (!AData.isEmpty())
			{
				vcard->setValueForTags(VVN_PHOTO_VALUE,AData.toBase64());
				vcard->setValueForTags(VVN_PHOTO_TYPE,QString("image/%1").arg(format));
			}
			else
			{
				vcard->setValueForTags(VVN_PHOTO_VALUE,QString::null);
				vcard->setValueForTags(VVN_PHOTO_TYPE,QString::null);
			}
			if (FVCardManager->publishVCard(AStreamJid,vcard))
			{
				published = true;
				LOG_STRM_INFO(AStreamJid,"Published self avatar in vCard");
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,"Failed to publish self avatar in vCard");
			}
			vcard->unlock();
		}
	}
	else
	{
		REPORT_ERROR("Failed to set self avatar: Invalid format");
	}
	return published;
}

QString Avatars::setCustomPictire(const Jid &AContactJid, const QByteArray &AData)
{
	if (!AData.isEmpty())
	{
		QString hash = saveAvatarData(AData);
		if (FCustomPictures.value(AContactJid) != hash)
		{
			LOG_INFO(QString("Changed custom picture for contact, jid=%1").arg(AContactJid.full()));
			FCustomPictures[AContactJid] = hash;
			updateDataHolder(AContactJid);
			emit avatarChanged(AContactJid);
		}
		return hash;
	}
	else if (FCustomPictures.contains(AContactJid))
	{
		LOG_INFO(QString("Removed custom picture for contact, jid=%1").arg(AContactJid.full()));
		FCustomPictures.remove(AContactJid);
		updateDataHolder(AContactJid);
		emit avatarChanged(AContactJid);
	}
	return EMPTY_AVATAR_HASH;
}

QImage Avatars::emptyAvatarImage(quint8 ASize, bool AGray) const
{
	QMap<quint8, QImage> &imagesCache = AGray ? FAvatarGrayImages[EMPTY_AVATAR_HASH] : FAvatarImages[EMPTY_AVATAR_HASH];
	if (!imagesCache.contains(ASize))
	{
		QImage colorImage, grayImage;
		NormalizeAvatarImage(FEmptyAvatar,ASize,colorImage,grayImage);
		storeAvatarImages(EMPTY_AVATAR_HASH,ASize,colorImage,grayImage);
		return AGray ? grayImage : colorImage;
	}
	return imagesCache.value(ASize);
}

QImage Avatars::cachedAvatarImage(const QString &AHash, quint8 ASize, bool AGray) const
{
	if (AHash == EMPTY_AVATAR_HASH)
		return emptyAvatarImage(ASize,AGray);
	else if (AGray)
		return FAvatarGrayImages.value(AHash).value(ASize);
	else
		return FAvatarImages.value(AHash).value(ASize);
}

QImage Avatars::loadAvatarImage(const QString &AHash, quint8 ASize, bool AGray) const
{
	QImage image = cachedAvatarImage(AHash,ASize,AGray);
	if (!AHash.isEmpty() && image.isNull() && hasAvatar(AHash))
	{
		QString fileName = avatarFileName(AHash);
		image = QImage::fromData(loadFileData(fileName));
		if (!image.isNull())
		{
			QImage colorImage, grayImage;
			NormalizeAvatarImage(image,ASize,colorImage,grayImage);
			storeAvatarImages(AHash,ASize,colorImage,grayImage);
			return AGray ? grayImage : colorImage;
		}
		else
		{
			REPORT_ERROR("Failed to load avatar image from file: Image not loaded");
			QFile::remove(fileName);
		}
	}
	return image;
}

QImage Avatars::visibleAvatarImage(const QString &AHash, quint8 ASize, bool AGray) const
{
	QImage image = loadAvatarImage(AHash,ASize,AGray);
	return image.isNull() ? emptyAvatarImage(ASize,AGray) : image;
}

QString Avatars::getImageFormat(const QByteArray &AData) const
{
	QBuffer buffer;
	buffer.setData(AData);
	buffer.open(QBuffer::ReadOnly);
	QByteArray format = QImageReader::imageFormat(&buffer);
	return QString::fromLocal8Bit(format.constData(),format.size());
}

QByteArray Avatars::loadFileData(const QString &AFileName) const
{
	if (!AFileName.isEmpty())
	{
		QFile file(AFileName);
		if (file.open(QFile::ReadOnly))
			return file.readAll();
		else if (file.exists())
			REPORT_ERROR(QString("Failed to load data from file: %1").arg(file.errorString()));
	}
	return QByteArray();
}

bool Avatars::saveFileData(const QString &AFileName, const QByteArray &AData) const
{
	if (!AFileName.isEmpty())
	{
		QFile file(AFileName);
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			file.write(AData);
			file.close();
			return true;
		}
		else
		{
			REPORT_ERROR(QString("Failed to save data to file: %1").arg(file.errorString()));
		}
	}
	return false;
}

void Avatars::storeAvatarImages(const QString &AHash, quint8 ASize, const QImage &AColor, const QImage &AGray) const
{
	FAvatarImages[AHash][ASize] = AColor;
	FAvatarGrayImages[AHash][ASize] = AGray;
}

bool Avatars::startLoadVCardAvatar(const Jid &AContactJid)
{
	if (FVCardManager)
	{
		QString vcardFile = FVCardManager->vcardFileName(AContactJid);
		if (QFile::exists(vcardFile))
		{
			LoadAvatarTask *task = new LoadAvatarTask(this,vcardFile,FAvatarSize,true);
			startLoadAvatarTask(AContactJid, task);
			return true;
		}
	}
	return false;
}

void Avatars::startLoadAvatarTask(const Jid &AContactJid, LoadAvatarTask *ATask)
{
	QHash<QString, LoadAvatarTask *>::iterator task_it = FFileTasks.find(ATask->FFile);
	if (task_it == FFileTasks.end())
	{
		LOG_DEBUG(QString("Load avatar task started, jid=%1, file=%2").arg(AContactJid.full(),ATask->FFile));
		FTaskContacts[ATask] += AContactJid;
		FFileTasks.insert(ATask->FFile, ATask);
		FThreadPool.start(ATask);
	}
	else
	{
		LOG_DEBUG(QString("Load avatar task merged, jid=%1, file=%2").arg(AContactJid.full(),ATask->FFile));
		FTaskContacts[task_it.value()] += AContactJid;
		delete ATask;
	}
}

void Avatars::updatePresence(const Jid &AStreamJid)
{
	IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen())
		presence->setPresence(presence->show(),presence->status(),presence->priority());
}

void Avatars::updateDataHolder(const Jid &AContactJid)
{
	if (FRostersModel)
	{
		QMultiMap<int,QVariant> findData;
		if (!AContactJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());
		foreach(int kind, AvatarRosterKinds)
			findData.insertMulti(RDR_KIND,kind);

		foreach(IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			emit rosterDataChanged(index,RDR_AVATAR_IMAGE);
	}
}

bool Avatars::updateVCardAvatar(const Jid &AContactJid, const QString &AHash, bool AFromVCard)
{
	// Update stream avatars
	for (QMap<Jid, QString>::iterator streamIt=FStreamAvatars.begin(); streamIt!=FStreamAvatars.end(); ++streamIt)
	{
		Jid streamJid = streamIt.key();
		if (!FBlockingResources.contains(streamJid) && streamJid.pBare()==AContactJid.pBare())
		{
			QString &curHash = streamIt.value();
			// Loaded stream avatar from vCard
			if (AFromVCard && curHash!=AHash)
			{
				LOG_STRM_INFO(streamJid,"Stream avatar changed");
				curHash = AHash;
				updatePresence(streamJid);
			}
			// Another resource changed avatar, request stream vCard
			else if (!AFromVCard && curHash!=AHash && curHash!=UNKNOWN_AVATAR_HASH)
			{
				LOG_STRM_INFO(streamJid,"Stream avatar set as unknown");
				curHash = UNKNOWN_AVATAR_HASH;
				updatePresence(streamJid);
				return false;
			}
		}
	}

	// Update users avatars
	QString &curHash = FVCardAvatars[AContactJid];
	if (curHash != AHash)
	{
		if (AHash.isEmpty() || hasAvatar(AHash))
		{
			LOG_DEBUG(QString("Contacts vCard avatar changed, jid=%1").arg(AContactJid.full()));

			curHash = AHash;
			updateDataHolder(AContactJid);
			emit avatarChanged(AContactJid);
		}
		else if (!AHash.isEmpty())
		{
			// Request vCard avatar
			return false;
		}
	}

	return true;
}

bool Avatars::updateIqAvatar(const Jid &AContactJid, const QString &AHash)
{
	QString &curHash = FIqAvatars[AContactJid];
	if (curHash != AHash)
	{
		if (AHash.isEmpty() || hasAvatar(AHash))
		{
			LOG_DEBUG(QString("Contact iq avatar changed, jid=%1").arg(AContactJid.full()));

			curHash = AHash;
			updateDataHolder(AContactJid);
			emit avatarChanged(AContactJid);
		}
		else if (!AHash.isEmpty())
		{
			// Request IQ avatar
			return false;
		}
	}
	return true;
}

bool Avatars::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	int singleKind = -1;
	foreach(IRosterIndex *index, ASelected)
	{
		int indexKind = index->kind();
		if (!AvatarRosterKinds.contains(indexKind))
			return false;
		else if (singleKind!=-1 && singleKind!=indexKind)
			return false;
		else if (!FStreamAvatars.contains(index->data(RDR_STREAM_JID).toString()))
			return false;
		singleKind = indexKind;
	}
	return !ASelected.isEmpty();
}

void Avatars::onVCardReceived(const Jid &AContactJid)
{
	startLoadVCardAvatar(AContactJid);
}

void Avatars::onVCardPublished(const Jid &AStreamJid)
{
	startLoadVCardAvatar(AStreamJid.bare());
}

void Avatars::onLoadAvatarTaskFinished(LoadAvatarTask *ATask)
{
	LOG_DEBUG(QString("Load avatar task finished, hash='%1', file=%2").arg(ATask->FHash,ATask->FFile));

	if (!ATask->FHash.isEmpty())
	{
		if (hasAvatar(ATask->FHash) || saveFileData(avatarFileName(ATask->FHash),ATask->FData))
			storeAvatarImages(ATask->FHash,ATask->FSize,ATask->FColorImage,ATask->FGrayImage);
	}

	foreach(const Jid &contactJid, FTaskContacts.value(ATask))
	{
		if (ATask->FVCard)
			updateVCardAvatar(contactJid,ATask->FHash,true);
		else
			updateDataHolder(contactJid);
	}

	FTaskContacts.remove(ATask);
	FFileTasks.remove(ATask->FFile);
	delete ATask;
}

void Avatars::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor && FVCardManager)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.order = SHO_PI_AVATARS;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_PRESENCE);
		FSHIPresenceIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionOut;
		FSHIPresenceOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.clear();
		shandle.conditions.append(SHC_IQ_AVATAR);
		FSHIIqAvatarIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
	FStreamAvatars.insert(AXmppStream->streamJid(),UNKNOWN_AVATAR_HASH);

	if (FVCardManager)
	{
		if (FVCardManager->requestVCard(AXmppStream->streamJid(),AXmppStream->streamJid().bare()))
			LOG_STRM_INFO(AXmppStream->streamJid(),"Load self avatar from vCard request sent");
		else
			LOG_STRM_WARNING(AXmppStream->streamJid(),"Failed to send load self avatar from vCard");
	}
}

void Avatars::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor && FVCardManager)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceOut.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIIqAvatarIn.take(AXmppStream->streamJid()));
	}
	FStreamAvatars.remove(AXmppStream->streamJid());
	FBlockingResources.remove(AXmppStream->streamJid());
}

void Avatars::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (AvatarRosterKinds.contains(AIndex->kind()))
	{
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if (!FVCardAvatars.contains(contactJid))
			startLoadVCardAvatar(contactJid);
	}
}

void Avatars::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void Avatars::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		int indexKind = AIndexes.first()->kind();
		QMap<int, QStringList> rolesMap = FRostersViewPlugin->rostersView()->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID);
		if (indexKind == RIK_STREAM_ROOT)
		{
			Menu *avatar = new Menu(AMenu);
			avatar->setTitle(tr("Avatar"));
			avatar->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CHANGE);

			Action *setup = new Action(avatar);
			setup->setText(tr("Set avatar"));
			setup->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_SET);
			setup->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
			connect(setup,SIGNAL(triggered(bool)),SLOT(onSetAvatarByAction(bool)));
			avatar->addAction(setup,AG_DEFAULT,false);

			Action *clear = new Action(avatar);
			clear->setText(tr("Clear avatar"));
			clear->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_REMOVE);
			clear->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
			connect(clear,SIGNAL(triggered(bool)),SLOT(onClearAvatarByAction(bool)));
			avatar->addAction(clear,AG_DEFAULT,false);

			AMenu->addAction(avatar->menuAction(),AG_RVCM_AVATARS_CHANGE,true);
		}
		else
		{
			Menu *picture = new Menu(AMenu);
			picture->setTitle(tr("Custom picture"));
			picture->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CHANGE);

			Action *setup = new Action(picture);
			setup->setText(tr("Set custom picture"));
			setup->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CUSTOM);
			setup->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
			connect(setup,SIGNAL(triggered(bool)),SLOT(onSetAvatarByAction(bool)));
			picture->addAction(setup,AG_DEFAULT,false);

			Action *clear = new Action(picture);
			clear->setText(tr("Clear custom picture"));
			clear->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_REMOVE);
			clear->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
			connect(clear,SIGNAL(triggered(bool)),SLOT(onClearAvatarByAction(bool)));
			picture->addAction(clear,AG_DEFAULT,false);

			AMenu->addAction(picture->menuAction(),AG_RVCM_AVATARS_CUSTOM,true);
		}
	}
}

void Avatars::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if ((ALabelId==AdvancedDelegateItem::DisplayId || ALabelId==FAvatarLabelId) && AvatarRosterKinds.contains(AIndex->kind()))
	{
		QString hash = avatarHash(AIndex->data(RDR_PREP_BARE_JID).toString());
		if (!hash.isEmpty() && hasAvatar(hash))
		{
			QString fileName = avatarFileName(hash);
			QSize imageSize = QImageReader(fileName).size();
			if (ALabelId!=FAvatarLabelId && (imageSize.height()>64 || imageSize.width()>64))
				imageSize.scale(QSize(64,64), Qt::KeepAspectRatio);
			QString avatarMask = "<img src='%1' width=%2 height=%3 />";
			AToolTips.insert(RTTO_AVATAR_IMAGE,avatarMask.arg(fileName).arg(imageSize.width()).arg(imageSize.height()));
		}
	}
}

void Avatars::onSetAvatarByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString fileName = QFileDialog::getOpenFileName(NULL, tr("Select avatar image"),QString::null,tr("Image Files (*.png *.jpg *.bmp *.gif)"));
		if (!fileName.isEmpty())
		{
			QByteArray data = loadFileData(fileName);
			if (!action->data(ADR_STREAM_JID).isNull())
			{
				foreach(const Jid &streamJid, action->data(ADR_STREAM_JID).toStringList())
					setAvatar(streamJid,data);
			}
			else if (!action->data(ADR_CONTACT_JID).isNull())
			{
				foreach(const Jid &contactJid, action->data(ADR_CONTACT_JID).toStringList())
					setCustomPictire(contactJid,data);
			}
		}
	}
}

void Avatars::onClearAvatarByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		if (!action->data(ADR_STREAM_JID).isNull())
		{
			foreach(const Jid &streamJid, action->data(ADR_STREAM_JID).toStringList())
				setAvatar(streamJid,QByteArray());
		}
		else if (!action->data(ADR_CONTACT_JID).isNull())
		{
			foreach(const Jid &contactJid, action->data(ADR_CONTACT_JID).toStringList())
				setCustomPictire(contactJid,QByteArray());
		}
	}
}

void Avatars::onIconStorageChanged()
{
	FAvatarImages.remove(EMPTY_AVATAR_HASH);
	FAvatarGrayImages.remove(EMPTY_AVATAR_HASH);
	FEmptyAvatar = QImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_AVATAR_EMPTY));
}

void Avatars::onOptionsOpened()
{
	QByteArray data = Options::fileValue("roster.avatars.custom-pictures").toByteArray();
	QDataStream stream(data);
	stream >> FCustomPictures;

	for (QMap<Jid,QString>::iterator it = FCustomPictures.begin(); it != FCustomPictures.end(); )
	{
		if (!hasAvatar(it.value()))
			it = FCustomPictures.erase(it);
		else
			++it;
	}

	onOptionsChanged(Options::node(OPV_ROSTER_VIEWMODE));
}

void Avatars::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FCustomPictures;
	Options::setFileValue(data,"roster.avatars.custom-pictures");

	FIqAvatars.clear();
	FVCardAvatars.clear();
	FCustomPictures.clear();
	FAvatarImages.clear();
	FAvatarGrayImages.clear();
}

void Avatars::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_VIEWMODE)
	{
		switch (ANode.value().toInt())
		{
		case IRostersView::ViewFull:
			FAvatarsVisible = true;
			FAvatarSize = avatarSize(AvatarNormal);
			break;
		case IRostersView::ViewSimple:
			FAvatarsVisible = true;
			FAvatarSize = avatarSize(AvatarSmall);
			break;
		case IRostersView::ViewCompact:
			FAvatarsVisible = false;
			FAvatarSize = avatarSize(AvatarSmall);
			break;
		}
		emit rosterLabelChanged(FAvatarLabelId,NULL);
	}
}
