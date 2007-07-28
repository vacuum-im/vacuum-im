#ifndef IPRIVATESTORAGE_H
#define IPRIVATESTORAGE_H

#define PRIVATESTORAGE_UUID "{E601766D-8867-47c5-B639-92DDEC224B33}"

#include <QDomDocument>
#include "../../utils/jid.h"

class IPrivateStorage
{
public:
  virtual QObject *instance() =0;

  virtual int saveElement(const Jid &AStreamJid, const QDomElement &AElement) =0;
  virtual int loadElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace, 
    bool ACheckCash = true) =0;
  virtual int removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) =0;
  virtual QDomElement getFromCash(const QString &ATagName, const QString &ANamespace) const =0;
signals:
  virtual void requestCompleted(int ARequestId) =0;
  virtual void requestError(int ARequestId) =0;
  virtual void requestTimeout(int ARequestId) =0;
};

Q_DECLARE_INTERFACE(IPrivateStorage,"Vacuum.Plugin.IPrivateStorage/1.0")

#endif