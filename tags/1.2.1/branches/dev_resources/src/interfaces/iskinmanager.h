#ifndef ISKINMANAGER_H
#define ISKINMANAGER_H

#define SKINMANAGER_UUID "{49D3C88B-8549-4a39-8B5F-B7E507791C4C}"

class ISkinManager
{
public:
  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(ISkinManager,"Vacuum.Plugin.ISkinManager/1.0")

#endif
