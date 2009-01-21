#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../interfaces/isettings.h"

class Settings : 
  public QObject,
  public ISettings
{
  Q_OBJECT;
  Q_INTERFACES(ISettings);
public:
  Settings(const QUuid &APluginId, ISettingsPlugin *ASettingsPlugin);
  ~Settings();
  //ISettings
  virtual QObject *instance() { return this; }
  virtual bool isSettingsOpened() const { return !FPluginNode.isNull(); }
  virtual QByteArray encript(const QString &AValue, const QByteArray &AKey) const;
  virtual QString decript(const QByteArray &AValue, const QByteArray &AKey) const;
  virtual QByteArray loadBinaryData(const QString &ADataId) const;
  virtual bool saveBinaryData(const QString &ADataId, const QByteArray &AData) const;
  virtual bool isValueNSExists(const QString &AName, const QString &ANameNS) const;
  virtual bool isValueExists(const QString &AName) const;
  virtual QVariant valueNS(const QString &AName, const QString &ANameNS, const QVariant &ADefault=QVariant()) const;
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) const;
  virtual QHash<QString,QVariant> values(const QString &AName) const;
  virtual ISettings &setValueNS(const QString &AName, const QString &ANameNS, const QVariant &AValue);
  virtual ISettings &setValue(const QString &AName, const QVariant &AValue);
  virtual ISettings &deleteValueNS(const QString &AName, const QString &ANameNS);
  virtual ISettings &deleteValue(const QString &AName);
  virtual ISettings &deleteNS(const QString &ANameNS);
public:
  void updatePluginNode();
protected:
  QDomElement getElement(const QString &AName, const QString &ANameNS, bool ACreate) const;
  static QString variantToString(const QVariant &AVariant);
  static QVariant stringToVariant(const QString &AString, QVariant::Type AType, const QVariant &ADefault);
  void delNSRecurse(const QString &ANameNS, QDomElement elem);
private:
  ISettingsPlugin *FSettingsPlugin;
private:
  QUuid FPluginId;
  QDomElement FPluginNode;
};

#endif // CONFIGURATOR_H
