#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <QObject>
#include <QDomDocument>
#include <QUuid>
#include <QVariant>
#include <QHash>

class ISettings {
public:
  virtual QObject* instance() =0;
  virtual QVariant valueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &ADefault=QVariant()) =0;
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) =0;
  virtual ISettings &setValueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &AValue) =0;
  virtual QHash<QString,QVariant> values(const QString &AName) =0;
  virtual ISettings &setValue(const QString &AName, const QVariant &AValue) =0;
  virtual ISettings &delValueNS(const QString &AName, const QString &ANameNS) =0;
  virtual ISettings &delValue(const QString &AName) =0;
  virtual ISettings &delNS(const QString &ANameNS) =0;
signals:
  virtual void opened()=0;
  virtual void closed()=0;
};

class ISettingsPlugin {
public:
  virtual QObject* instance() =0;
  virtual ISettings *newSettings(const QUuid &, QObject *)=0;
  virtual QString fileName() const =0;
  virtual bool setFileName(const QString &) =0;
  virtual bool saveSettings() =0;
  virtual QDomDocument document() =0;
  virtual QString profile() const =0;
  virtual QDomElement setProfile(const QString &) =0;
  virtual QDomElement getProfile(const QString &) =0;
  virtual QDomElement getPluginNode(const QUuid &) =0;
signals:
  virtual void profileOpened()=0;
  virtual void profileClosed()=0;
};

Q_DECLARE_INTERFACE(ISettings,"Vacuum.Plugin.ISettings/1.0")
Q_DECLARE_INTERFACE(ISettingsPlugin,"Vacuum.Plugin.ISettingsPlugin/1.0")

#endif
