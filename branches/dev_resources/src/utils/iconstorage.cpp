#include "iconstorage.h"

#include <QFile>
#include <QImage>
#include <QVariant>
#include <QApplication>

QHash<QString, QHash<QString,QIcon> > IconStorage::FIconCach;
QHash<QString, IconStorage*> IconStorage::FStaticStorages;
QHash<QObject*, IconStorage*> IconStorage::FObjectStorage;

IconStorage::IconStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : FileStorage(AStorage,ASubStorage,AParent)
{
  connect(this,SIGNAL(storageChanged()),SLOT(onStorageChanged()));
}

IconStorage::~IconStorage()
{
  QList<QObject*> objects = FUpdateParams.keys();
  foreach(QObject *object, objects) {
    removeObject(object); }
}

QIcon IconStorage::getIcon(const QString AKey, int AIndex) const
{
  QIcon icon;
  QString file = fileName(AKey,AIndex);
  if (!file.isEmpty())
  {
    icon = FIconCach[subStorage()].value(file);
    if (icon.isNull())
    {
      QString filePath = storageRootDir() + file;
      if (QFile::exists(filePath))
      {
        icon.addFile(filePath);
        FIconCach[subStorage()].insert(file,icon);
      }
    }
  }
  return icon;
}

void IconStorage::clearIconCach()
{
  FIconCach.clear();
}

IconStorage *IconStorage::staticStorage(const QString &AStorage)
{
  IconStorage *storage = FStaticStorages.value(AStorage,NULL);
  if (!storage)
  {
    storage = new IconStorage(AStorage,"",qApp);
    FStaticStorages.insert(AStorage,storage);
  }
  return storage;
}

void IconStorage::insertAutoIcon(QObject *AObject, const QString AKey, int AIndex, int AAnimate, const QString &AProperty)
{
  IconStorage *oldStorage = FObjectStorage.value(AObject);
  if (oldStorage!=NULL && oldStorage!=this)
    oldStorage->removeAutoIcon(AObject);

  if (AObject!=NULL && !AKey.isEmpty())
  {
    IconUpdateParams *params;
    if (oldStorage!=this)
    {
      params = new IconUpdateParams;
      FObjectStorage.insert(AObject,this);
      FUpdateParams.insert(AObject,params);
    }
    else
    {
      params = FUpdateParams.value(AObject);
    }
    params->key = AKey;
    params->index = AIndex;
    params->prop = AProperty;
    params->animate = AAnimate;
    QString file = fileFullName(AKey,AIndex);
    if (!file.isEmpty())
      params->size = QImageReader(file).size();
    initAnimation(AObject,params);
    updateObject(AObject);
    connect(AObject,SIGNAL(destroyed(QObject *)),SLOT(onObjectDestroyed(QObject *)));
  }
  else if (AObject!=NULL)
  {
    removeAutoIcon(AObject);
  }
}

void IconStorage::removeAutoIcon(QObject *AObject)
{
  if (FUpdateParams.contains(AObject))
  {
    removeObject(AObject);
    disconnect(AObject,SIGNAL(destroyed(QObject *)),this,SLOT(onObjectDestroyed(QObject *)));
  }
}

void IconStorage::initAnimation(QObject *AObject, IconUpdateParams *AParams)
{
  removeAnimation(AObject,AParams);
  if (AParams->animate >= 0)
  {
    if (AParams->animation == NULL)
    {
      int iconCount = filesCount(AParams->key);
      QString file = fileFullName(AParams->key,AParams->index);
      if (iconCount > 1)
      {
        int interval = AParams->animate > 0 ? AParams->animate : fileOption(AParams->key,OPTION_ANIMATE).toInt();
        if (interval > 0)
        {
          AParams->animation = new IconAnimateParams;
          AParams->animation->frameCount = iconCount;
          AParams->animation->timer.setSingleShot(false);
          AParams->animation->timer.setInterval(interval);
        }
      }
      else if (!file.isEmpty() && QImageReader(file).supportsAnimation())
      {
        AParams->animation = new IconAnimateParams;
        AParams->animation->reader = new QImageReader(file);
      }
      if (AParams->animation)
      {
        FTimerObjects.insertMulti(&AParams->animation->timer,AObject);
        connect(&AParams->animation->timer,SIGNAL(timeout()),SLOT(onAnimateTimer()));
        AParams->animation->timer.start();
      }
    }
    else
    {
      FTimerObjects.insertMulti(&AParams->animation->timer,AObject);
    }
  }
}

void IconStorage::removeAnimation(QObject *AObject, IconUpdateParams *AParams)
{
  if (AParams && AParams->animation)
  {
    FTimerObjects.remove(&AParams->animation->timer,AObject);
    if (FTimerObjects.count(&AParams->animation->timer) == 0)
    {
      delete AParams->animation;
      AParams->animation = NULL;
    }
  }
}

void IconStorage::updateObject(QObject *AObject)
{
  QIcon icon;
  IconUpdateParams *params = FUpdateParams[AObject];
  if (params->animation)
  {
    if (params->animation->reader)
    {
      QImage image = params->animation->reader->read();
      if (image.isNull())
      {
        params->animation->reader->setFileName(params->animation->reader->fileName());
        image = params->animation->reader->read();
      }
      icon.addPixmap(QPixmap::fromImage(image));
      params->animation->timer.start(params->animation->reader->nextImageDelay());
    }
    else
    {
      icon = getIcon(params->key,params->animation->frameIndex);
    }
  }
  else
  {
    icon = getIcon(params->key,params->index);
  }
  if (params->prop == "pixmap")
    AObject->setProperty(params->prop.toLatin1(),icon.pixmap(params->size));
  else
    AObject->setProperty(params->prop.toLatin1(),icon);
}

void IconStorage::removeObject(QObject *AObject)
{
  FObjectStorage.remove(AObject);
  IconUpdateParams *params = FUpdateParams.take(AObject);
  removeAnimation(AObject,params);
  delete params;
}

void IconStorage::onStorageChanged()
{
  FTimerObjects.clear();
  for (QHash<QObject*,IconUpdateParams*>::iterator it=FUpdateParams.begin(); it!=FUpdateParams.end(); it++)
  {
    initAnimation(it.key(),it.value());
    updateObject(it.key());
  }
}

void IconStorage::onAnimateTimer()
{
  QTimer *timer = qobject_cast<QTimer *>(sender());
  foreach(QObject *object, FTimerObjects.values(timer))
  {
    IconUpdateParams *params = FUpdateParams[object];
    params->animation->frameIndex = params->animation->frameCount>0 ? (params->animation->frameIndex + 1) % params->animation->frameCount : 0;
    updateObject(object);
  }
}

void IconStorage::onObjectDestroyed(QObject *AObject)
{
  removeObject(AObject);
}

