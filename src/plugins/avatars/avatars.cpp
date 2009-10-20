#include "avatars.h"

#include <QFile>
#include <QTimer>
#include <QBuffer>
#include <QFileDialog>
#include <QImageReader>
#include <QCryptographicHash>

#define DIR_AVATARS               "avatars"

#define SHC_PRESENCE              "/presence"
#define SHC_IQ_AVATAR             "/iq[@type='get']/query[@xmlns='" NS_JABBER_IQ_AVATAR "']"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

#define SVN_SHOW_AVATARS          "showAvatar"
#define SVN_AVATAR_ALIGNLEFT      "avatarAlignLeft"
#define SVN_AVATAR_SIZE           "avatarSize"
#define SVN_CUSTOM_AVATARS        "customAvatars"
#define SVN_CUSTOM_AVATAR_HASH    SVN_CUSTOM_AVATARS":hash[]"

#define IFN_EMPTY_AVATAR          "logo_32.png"

#define AVATAR_IMAGE_TYPE         "jpeg"
#define AVATAR_IQ_TIMEOUT         30000

Avatars::Avatars()
{
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FVCardPlugin = NULL;
  FPresencePlugin = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FSettingsPlugin = NULL;
  
  FRosterLabelId = -1;
  FOptions = IAvatars::ShowAvatars;
}

Avatars::~Avatars()
{

}

void Avatars::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Avatars manager"); 
  APluginInfo->description = tr("Enable a user to store and retrieve an avatar");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(VCARD_UUID);
}

bool Avatars::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
    if (FRostersModel)
    {
      connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)), SLOT(onRosterIndexInserted(IRosterIndex *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (FRostersViewPlugin)
    {
      connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
        SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
      connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
        SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return true;
}

bool Avatars::initObjects()
{
  if (FRostersModel)
  {
    FRostersModel->insertDefaultDataHolder(this);
  }
  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
  }

  FCurOptions = 0;
  onUpdateOptions();
  return true;
}

bool Avatars::stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &/*AAccept*/)
{
  if (FSHIPresenceOut.value(AStreamJid) == AHandlerId)
  {
    QDomElement vcardUpdate = AStanza.addElement("x",NS_VCARD_UPDATE);
    if (!FStreamAvatars.value(AStreamJid).isNull())
    {
      QDomElement photoElem = vcardUpdate.appendChild(AStanza.createElement("photo")).toElement();
      photoElem.appendChild(AStanza.createTextNode(FStreamAvatars.value(AStreamJid)));
    }

    if (!FStreamAvatars.value(AStreamJid).isEmpty())
    {
      QDomElement iqUpdate = AStanza.addElement("x",NS_JABBER_X_AVATAR);
      QDomElement hashElem = iqUpdate.appendChild(AStanza.createElement("hash")).toElement();
      hashElem.appendChild(AStanza.createTextNode(FStreamAvatars.value(AStreamJid)));
    }
  }
  return false;
}

bool Avatars::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHIPresenceIn.value(AStreamJid)==AHandlerId && AStanza.type().isEmpty())
  {
    Jid contactJid = AStanza.from();
    if (AStreamJid!=contactJid && AStanza.firstElement("x",NS_MUC_USER).isNull())
    {
      QDomElement vcardUpdate = AStanza.firstElement("x",NS_VCARD_UPDATE);
      QDomElement iqUpdate = AStanza.firstElement("x",NS_JABBER_X_AVATAR);
      if (!vcardUpdate.isNull())
      {
        QString hash = vcardUpdate.firstChildElement("photo").text().toLower();
        if (!updateVCardAvatar(contactJid.bare(),hash))
          FVCardPlugin->requestVCard(AStreamJid,contactJid.bare());
      }
      else if (!iqUpdate.isNull())
      {
        QString hash = iqUpdate.firstChildElement("hash").text().toLower();
        if (!updateIqAvatar(contactJid,hash))
        {
          Stanza query("iq");
          query.setTo(contactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
          query.addElement("query",NS_JABBER_IQ_AVATAR);
          if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,query,AVATAR_IQ_TIMEOUT))
            FIqAvatarRequests.insert(query.id(),contactJid);
          else
            FIqAvatars.remove(contactJid);
        }
      }
      else if (!FIqAvatars.value(contactJid).isEmpty())
      {
        updateIqAvatar(contactJid,"");
      }
    }
  }
  else if (FSHIIqAvatarIn.value(AStreamJid) == AHandlerId)
  {
    QFile file(avatarFileName(FStreamAvatars.value(AStreamJid)));
    if (file.open(QFile::ReadOnly))
    {
      AAccept = true;
      Stanza result("iq");
      result.setTo(AStanza.from()).setType("result").setId(AStanza.id());
      QDomElement dataElem = result.addElement("query",NS_JABBER_IQ_AVATAR).appendChild(result.createElement("data")).toElement();
      dataElem.appendChild(result.createTextNode(file.readAll().toBase64()));
      FStanzaProcessor->sendStanzaOut(AStreamJid,result);
      file.close();
    }
  }
  return false;
}

void Avatars::stanzaRequestResult(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FIqAvatarRequests.contains(AStanza.id()))
  {
    Jid contactJid = FIqAvatarRequests.take(AStanza.id());
    if (AStanza.type() == "result")
    {
      QDomElement dataElem = AStanza.firstElement("query",NS_JABBER_IQ_AVATAR).firstChildElement("data");
      QByteArray avatarData = QByteArray::fromBase64(dataElem.text().toAscii());
      if (!avatarData.isEmpty())
      {
        QString hash = saveAvatar(avatarData);
        updateIqAvatar(contactJid,hash);
      }
      else
        FIqAvatars.remove(contactJid);
    }
    else
      FIqAvatars.remove(contactJid);
  }
}

void Avatars::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
  Q_UNUSED(AStreamJid);
  if (FIqAvatarRequests.contains(AStanzaId))
  {
    Jid contactJid = FIqAvatars.take(AStanzaId);
    FIqAvatars.remove(contactJid);
  }
}

int Avatars::order() const
{
  return RDHO_DEFAULT;
}

QList<int> Avatars::roles() const
{
  static const QList<int> indexRoles = QList<int>() << RDR_AVATAR_HASH << RDR_AVATAR_IMAGE;
  return indexRoles;
}

QList<int> Avatars::types() const
{
  static const QList<int> indexTypes = QList<int>() << RIT_STREAM_ROOT << RIT_CONTACT;
  return indexTypes;
}

QVariant Avatars::data(const IRosterIndex *AIndex, int ARole) const
{
  if (ARole == RDR_AVATAR_IMAGE)
    return avatarImage(AIndex->data(RDR_JID).toString());
  else if (ARole == RDR_AVATAR_HASH)
    return avatarHash(AIndex->data(RDR_JID).toString());
  return QVariant();
}

bool Avatars::setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/)
{
  return false;
}

QWidget *Avatars::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_ROSTER)
  {
    AOrder = OWO_ROSTER_AVATARS;
    RosterOptionsWidget *widget = new RosterOptionsWidget(this,NULL);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

QString Avatars::avatarFileName(const QString &AHash) const
{
  return !AHash.isEmpty() ? FAvatarsDir.filePath(AHash.toLower()) : QString::null;
}

bool Avatars::hasAvatar(const QString &AHash) const
{
  return !AHash.isEmpty() && QFile::exists(FAvatarsDir.filePath(AHash));
}

QImage Avatars::loadAvatar(const QString &AHash) const
{
  return QImage(avatarFileName(AHash));
}

QString Avatars::saveAvatar(const QByteArray &AImageData) const
{
  if (!AImageData.isEmpty())
  {
    QString hash = QCryptographicHash::hash(AImageData,QCryptographicHash::Sha1).toHex();
    if (!hasAvatar(hash))
    {
      QFile file(avatarFileName(hash));
      if (file.open(QFile::WriteOnly|QFile::Truncate))
      {
        file.write(AImageData);
        file.close();
        return hash;
      }
    }
    else
      return hash;
  }
  return QString();
}

QString Avatars::saveAvatar(const QImage &AImage, const char *AFormat) const
{
  QByteArray bytes;
  QBuffer buffer(&bytes);
  return AImage.save(&buffer,AFormat) ? saveAvatar(bytes) : QString();
}

QString Avatars::avatarHash(const Jid &AContactJid) const
{
  QString hash = FCustomPictures.value(AContactJid.bare());
  if (hash.isEmpty())
    hash = FIqAvatars.value(AContactJid);
  if (hash.isEmpty())
    hash = FVCardAvatars.value(AContactJid.bare());
  return hash;
}

QImage Avatars::avatarImage(const Jid &AContactJid) const
{
  QString hash = avatarHash(AContactJid);
  if (!hash.isEmpty() && !FAvatarImages.contains(hash))
  {
    QString fileName = avatarFileName(avatarHash(AContactJid));
    if (QFile::exists(fileName))
    {
      QImage image = QImage(fileName).scaled(FAvatarSize,Qt::KeepAspectRatio,Qt::FastTransformation);
      if (!image.isNull())
        FAvatarImages.insert(hash,image);
      return image;
    }
  }
  return FAvatarImages.value(hash,FEmptyAvatar);
}

void Avatars::setAvatarSize(const QSize &ASize)
{
  if (FAvatarSize != ASize)
  {
    if (ASize.width() > 96 || ASize.height()>96)
      FAvatarSize = QSize(96,96);
    else if (ASize.width()<32 || ASize.height()<32)
      FAvatarSize = QSize(32,32);
    else
      FAvatarSize = ASize;
    //FEmptyAvatar = QImage(FAvatarSize,QImage::Format_RGB32);
    FAvatarImages.clear();
    updateDataHolder();
  }
}

bool Avatars::setAvatar(const Jid &AStreamJid, const QImage &AImage, const char *AFormat)
{
  bool published = false;
  IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AStreamJid.bare()) : NULL;
  if (vcard)
  {
    const static QSize maxSize = QSize(96,96);
    QImage avatar = AImage.width()>96 || AImage.height()>96 ? AImage.scaled(QSize(96,96),Qt::KeepAspectRatio,Qt::SmoothTransformation) : AImage;
    vcard->setPhotoImage(avatar, AFormat);
    published = FVCardPlugin->publishVCard(vcard,AStreamJid);
    vcard->unlock();
  }
  return published;
}

QString Avatars::setCustomPictire(const Jid &AContactJid, const QString &AImageFile)
{
  if (!AImageFile.isEmpty())
  {
    QFile file(AImageFile);
    if (file.open(QFile::ReadOnly))
    {
      QString hash = saveAvatar(file.readAll());
      if (FCustomPictures.value(AContactJid)!=hash)
      {
        FCustomPictures[AContactJid] = hash;
        updateDataHolder(AContactJid);
      }
      file.close();
      return hash;
    }
  }
  else
  {
    FCustomPictures.remove(AContactJid);
    updateDataHolder(AContactJid);
    return "";
  }
  return QString();
}

bool Avatars::checkOption(IAvatars::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void Avatars::setOption(IAvatars::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    QTimer::singleShot(0,this,SLOT(onUpdateOptions()));
    emit optionChanged(AOption,AValue);
  }
}

QByteArray Avatars::loadAvatarFromVCard(const Jid &AContactJid) const
{
  if (FVCardPlugin)
  {
    QDomDocument vcard;
    QFile file(FVCardPlugin->vcardFileName(AContactJid));
    if (file.open(QFile::ReadOnly) && vcard.setContent(&file,true))
    {
      QDomElement binElem = vcard.documentElement().firstChildElement("vCard").firstChildElement("PHOTO").firstChildElement("BINVAL");
      if (!binElem.isNull())
        return QByteArray::fromBase64(binElem.text().toLatin1());
    }
  }
  return QByteArray();
}

void Avatars::updatePresence(const Jid &AStreamJid) const
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
    presence->setPresence(presence->show(),presence->status(),presence->priority());
}

void Avatars::updateDataHolder(const Jid &AContactJid)
{
  if (FRostersModel)
  {
    QMultiHash<int,QVariant> findData;
    foreach(int type, types())
      findData.insert(RDR_TYPE,type);
    if (!AContactJid.isEmpty())
      findData.insert(RDR_BARE_JID,AContactJid.pBare());
    IRosterIndexList indexes = FRostersModel->rootIndex()->findChild(findData,true);
    foreach (IRosterIndex *index, indexes)
    {
      emit dataChanged(index,RDR_AVATAR_HASH);
      emit dataChanged(index,RDR_AVATAR_IMAGE);
    }
  }
}

bool Avatars::updateVCardAvatar(const Jid &AContactJid, const QString &AHash)
{
  foreach(Jid streamJid, FStreamAvatars.keys())
  {
    if ((AContactJid && streamJid) && FStreamAvatars.value(streamJid)!=AHash)
    {
      FStreamAvatars[streamJid] = AHash;
      updatePresence(streamJid);
    }
  }
  if (FVCardAvatars.value(AContactJid) != AHash)
  {
    FVCardAvatars[AContactJid] = AHash;
    if (AHash.isEmpty() || hasAvatar(AHash))
    {
      updateDataHolder(AContactJid);
      emit avatarChanged(AContactJid);
    }
    else if (!AHash.isEmpty())
    {
      return false;
    }
  }
  return true;
}

bool Avatars::updateIqAvatar(const Jid &AContactJid, const QString &AHash)
{
  if (FIqAvatars.value(AContactJid) != AHash)
  {
    FIqAvatars[AContactJid] = AHash;
    if (AHash.isEmpty() || hasAvatar(AHash))
    {
      updateDataHolder(AContactJid);
      emit avatarChanged(AContactJid);
    }
    else if (!AHash.isEmpty())
    {
      return false;
    }
  }
  return true;
}

QImage Avatars::convertToGray(const QImage &AImage) const
{
  if (!AImage.isNull())
  {
    QImage image = AImage.convertToFormat(QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) 
    {
      for (int x = 0; x < image.width(); ++x) 
      {
        QRgb pixel = image.pixel(x, y);
        int gray = qGray(pixel);
        int alpha = qAlpha(pixel);
        image.setPixel(x, y, qRgba(gray, gray, gray, alpha));
      }
    }
    return image;
  }
  return QImage();
}

void Avatars::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FVCardPlugin)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.streamJid = AXmppStream->streamJid();
    
    shandle.priority = SHP_AVATARS_PRESENCE;
    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.conditions.append(SHC_PRESENCE);
    FSHIPresenceIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.priority = SHP_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionOut;
    FSHIPresenceOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.priority = SHP_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.conditions.clear();
    shandle.conditions.append(SHC_IQ_AVATAR);
    FSHIIqAvatarIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
  }
  FStreamAvatars.insert(AXmppStream->streamJid(),QString::null);

  if (FVCardPlugin)
  {
    FVCardPlugin->requestVCard(AXmppStream->streamJid(),AXmppStream->streamJid().bare());
  }
}

void Avatars::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FVCardPlugin)
  {
    FStanzaProcessor->removeStanzaHandle(FSHIPresenceIn.take(AXmppStream->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIPresenceOut.take(AXmppStream->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIIqAvatarIn.take(AXmppStream->streamJid()));
  }
  FStreamAvatars.remove(AXmppStream->streamJid());
}

void Avatars::onVCardChanged(const Jid &AContactJid)
{
  QString hash = saveAvatar(loadAvatarFromVCard(AContactJid));
  updateVCardAvatar(AContactJid,hash);
}

void Avatars::onRosterIndexInserted(IRosterIndex *AIndex)
{
  if (FRostersViewPlugin && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->data(RDR_BARE_JID).toString();
    if (!FVCardAvatars.contains(contactJid))
      onVCardChanged(contactJid);
    if (checkOption(IAvatars::ShowAvatars))
      FRostersViewPlugin->rostersView()->insertIndexLabel(FRosterLabelId, AIndex);
  }
}

void Avatars::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_STREAM_ROOT && FStreamAvatars.contains(AIndex->data(RDR_STREAM_JID).toString()))
  {
    Menu *avatar = new Menu(AMenu);
    avatar->setTitle(tr("Avatar"));
    avatar->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CHANGE);

    Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
    Action *setup = new Action(avatar);
    setup->setText(tr("Set avatar"));
    setup->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_SET);
    setup->setData(ADR_STREAM_JID,streamJid.full());
    connect(setup,SIGNAL(triggered(bool)),SLOT(onSetAvatarByAction(bool)));
    avatar->addAction(setup,AG_DEFAULT,false);

    Action *clear = new Action(avatar);
    clear->setText(tr("Clear avatar"));
    clear->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_REMOVE);
    clear->setData(ADR_STREAM_JID,streamJid.full());
    clear->setEnabled(!FStreamAvatars.value(streamJid).isEmpty());
    connect(clear,SIGNAL(triggered(bool)),SLOT(onClearAvatarByAction(bool)));
    avatar->addAction(clear,AG_DEFAULT,false);

    AMenu->addAction(avatar->menuAction(),AG_RVCM_AVATARS,true);
  }
  else if (AIndex->type() == RIT_CONTACT)
  {
    Menu *picture = new Menu(AMenu);
    picture->setTitle(tr("Custom picture"));
    picture->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CHANGE);

    Jid contactJid = AIndex->data(RDR_JID).toString();
    Action *setup = new Action(picture);
    setup->setText(tr("Set custom picture"));
    setup->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_CUSTOM);
    setup->setData(ADR_CONTACT_JID,contactJid.bare());
    connect(setup,SIGNAL(triggered(bool)),SLOT(onSetAvatarByAction(bool)));
    picture->addAction(setup,AG_DEFAULT,false);

    Action *clear = new Action(picture);
    clear->setText(tr("Clear custom picture"));
    clear->setIcon(RSR_STORAGE_MENUICONS,MNI_AVATAR_REMOVE);
    clear->setData(ADR_CONTACT_JID,contactJid.bare());
    clear->setEnabled(FCustomPictures.contains(contactJid.bare()));
    connect(clear,SIGNAL(triggered(bool)),SLOT(onClearAvatarByAction(bool)));
    picture->addAction(clear,AG_DEFAULT,false);

    AMenu->addAction(picture->menuAction(),AG_RVCM_AVATARS,true);
  }
}

void Avatars::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if ((ALabelId == RLID_DISPLAY || ALabelId == FRosterLabelId) && types().contains(AIndex->type()))
  {
    QString hash = AIndex->data(RDR_AVATAR_HASH).toString();
    if (hasAvatar(hash))
    {
      QString fileName = avatarFileName(hash);
      QSize imageSize = QImageReader(fileName).size();
      imageSize.scale(ALabelId==FRosterLabelId ? QSize(128,128) : QSize(64,64),Qt::KeepAspectRatio);
      QString avatarMask = "<img src='%1' width=%2 height=%3>";
      AToolTips.insert(RTTO_AVATAR_IMAGE,avatarMask.arg(fileName).arg(imageSize.width()).arg(imageSize.height()));
    }
  }
}

void Avatars::onSetAvatarByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString fileName = QFileDialog::getOpenFileName(NULL, tr("Select avatar image"),"",tr("Image Files (*.png *.jpg *.bmp *.gif)"));
    if (!fileName.isEmpty())
    {
      if (!action->data(ADR_STREAM_JID).isNull())
      {
        Jid streamJid = action->data(ADR_STREAM_JID).toString();
        setAvatar(streamJid,QImage(fileName),AVATAR_IMAGE_TYPE);
      }
      else if (!action->data(ADR_CONTACT_JID).isNull())
      {
        Jid contactJid = action->data(ADR_CONTACT_JID).toString();
        setCustomPictire(contactJid,fileName);
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
      Jid streamJid = action->data(ADR_STREAM_JID).toString();
      setAvatar(streamJid,QImage());
    }
    else if (!action->data(ADR_CONTACT_JID).isNull())
    {
      Jid contactJid = action->data(ADR_CONTACT_JID).toString();
      setCustomPictire(contactJid,"");
    }
  }
}

void Avatars::onSettingsOpened()
{
  FCustomPictures.clear();
  FIqAvatars.clear();
  FVCardAvatars.clear();
  FAvatarImages.clear();

  FAvatarsDir = FSettingsPlugin->homeDir();
  if (!FAvatarsDir.exists(DIR_AVATARS))
    FAvatarsDir.mkdir(DIR_AVATARS);
  FAvatarsDir.cd(DIR_AVATARS);

  ISettings *settings = FSettingsPlugin->settingsForPlugin(AVATARTS_UUID);
  setOption(IAvatars::ShowAvatars,settings->value(SVN_SHOW_AVATARS,true).toBool()); 
  setOption(IAvatars::AvatarsAlignLeft,settings->value(SVN_AVATAR_ALIGNLEFT,false).toBool()); 
  setAvatarSize(settings->value(SVN_AVATAR_SIZE,QSize(32,32)).toSize());

  QHash<QString,QVariant> customPictires = settings->values(SVN_CUSTOM_AVATAR_HASH);
  for (QHash<QString,QVariant>::const_iterator it = customPictires.constBegin(); it != customPictires.constEnd(); it++)
  {
    if (hasAvatar(it.value().toString()))
    {
      Jid contactJid = it.key();
      FCustomPictures.insert(contactJid,it.value().toString());
    }
  }
}

void Avatars::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(AVATARTS_UUID);
  settings->setValue(SVN_SHOW_AVATARS,checkOption(IAvatars::ShowAvatars));
  settings->setValue(SVN_AVATAR_ALIGNLEFT,checkOption(IAvatars::AvatarsAlignLeft));
  settings->setValue(SVN_AVATAR_SIZE,FAvatarSize);

  settings->deleteValue(SVN_CUSTOM_AVATARS);
  QList<Jid> contacts = FCustomPictures.keys();
  foreach(Jid contactJid, contacts)
  {
    settings->setValueNS(SVN_CUSTOM_AVATAR_HASH,contactJid.full(),FCustomPictures.value(contactJid));
  }
}

void Avatars::onUpdateOptions()
{
  if (FRostersViewPlugin && FRostersModel && FOptions != FCurOptions)
  {
    FRostersViewPlugin->rostersView()->destroyIndexLabel(FRosterLabelId);
    FRosterLabelId = -1;
    if (checkOption(IAvatars::ShowAvatars))
    {
      int labelOrder = checkOption(IAvatars::AvatarsAlignLeft) ? RLO_AVATAR_IMAGE_LEFT : RLO_AVATAR_IMAGE_RIGHT;
      FRosterLabelId = FRostersViewPlugin->rostersView()->createIndexLabel(labelOrder,RDR_AVATAR_IMAGE);
      QMultiHash<int,QVariant> findData;
      foreach(int type, types())
        findData.insertMulti(RDR_TYPE,type);
      IRosterIndexList indexes = FRostersModel->rootIndex()->findChild(findData,true);
      foreach (IRosterIndex *index, indexes)
        FRostersViewPlugin->rostersView()->insertIndexLabel(FRosterLabelId,index);
    }
    FCurOptions = FOptions;
  }
}

Q_EXPORT_PLUGIN2(AvatarsPlugin, Avatars)
