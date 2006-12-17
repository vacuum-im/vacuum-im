#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

#include "interfaces/isettings.h"

class Settings : 
  public QObject,
  public ISettings
{
  Q_OBJECT;
  Q_INTERFACES(ISettings);

public:
  Settings(const QUuid &AUuid, ISettingsPlugin *ASettingsPlugin, QObject *parent);
  ~Settings();

  //ISettings
  virtual QObject *instance() { return this; }
  virtual QVariant valueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &ADefault=QVariant());
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant());
  virtual QHash<QString,QVariant> values(const QString &AName);
  virtual ISettings &setValueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &AValue);
  virtual ISettings &setValue(const QString &AName, const QVariant &AValue);
  virtual ISettings &delValueNS(const QString &AName, const QString &ANameNS);
  virtual ISettings &delValue(const QString &AName);
  virtual ISettings &delNS(const QString &ANameNS);
signals:
  virtual void opened();
  virtual void closed();
protected:
  QDomElement getElement(const QString &AName, const QString &ANameNS, bool ACreate);
  void delNSRecurse(const QString &ANameNS, QDomNode node);
private slots:
  virtual void onProfileOpened();
  virtual void onProfileClosed();
private:
  ISettingsPlugin *FSettingsPlugin;
private:
  QUuid FUuid;
  QDomElement FSettings;
  bool FSettingsOpened;
};

#endif // CONFIGURATOR_H
