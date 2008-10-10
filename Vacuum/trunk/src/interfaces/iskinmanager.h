#ifndef ISKINMANAGER_H
#define ISKINMANAGER_H

#include "../../utils/iconset.h"

#define SKINMANAGER_UUID "{49D3C88B-8549-4a39-8B5F-B7E507791C4C}"


class ISkinManager
{
public:
  virtual QObject *instance() =0;
  virtual void setSkin(const QString &ASkin) =0;
  virtual void setSkinsDirectory(const QString &ASkinsDirectory) =0;
public slots:
  virtual void setSkinByAction(bool) =0;
signals:
  virtual void skinDirectoryChanged() =0;
  virtual void skinAboutToBeChanged() =0;
  virtual void skinChanged() =0;
};

Q_DECLARE_INTERFACE(ISkinManager,"Vacuum.Plugin.ISkinManager/1.0")

#endif
