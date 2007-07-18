#ifndef IPRIVATESTORAGE_H
#define IPRIVATESTORAGE_H

#define PRIVATESTORAGE_UUID "{E601766D-8867-47c5-B639-92DDEC224B33}"

class IPrivateStorage
{
public:
  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IPrivateStorage,"Vacuum.Plugin.IPrivateStorage/1.0")

#endif