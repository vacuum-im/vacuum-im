#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <QStringList>
#include <QDomDocument>
#include <QUuid>
#include <QVariant>
#include <QHash>
#include <QWidget>
#include <QIcon>
#include <QDir>

#define SETTINGS_UUID "{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"

class IOptionsHolder {
public:
  virtual QObject *instance() =0;
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder) const =0;
signals:
  virtual void optionsAccepted() =0;
  virtual void optionsRejected() =0;
};

class ISettings {
public:
  virtual QObject* instance() =0;
  virtual QByteArray encript(const QString &AValue, const QByteArray &AKey) const =0;
  virtual QString decript(const QByteArray &AValue, const QByteArray &AKey) const =0;
  virtual QVariant valueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &ADefault=QVariant()) const =0;
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) const =0;
  virtual QHash<QString,QVariant> values(const QString &AName) const =0;
  virtual ISettings &setValueNS(const QString &AName, const QString &ANameNS, 
    const QVariant &AValue) =0;
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
  virtual QObject *instance() =0;
  virtual bool isProfilesValid() const =0;
  virtual bool isProfileOpened() const =0;
  virtual const QDir &homeDir() const =0;
  virtual const QDir &profileDir() const =0;
  virtual ISettings *openSettings(const QUuid &APluginId, QObject *AParent)=0;
  virtual bool saveSettings() =0;
  virtual bool addProfile(const QString &AProfile) =0;
  virtual QString profile() const =0;
  virtual QStringList profiles() const =0;
  virtual QDomElement profileNode(const QString &AProfile) =0;
  virtual QDomElement pluginNode(const QUuid &APluginId) =0;
  virtual bool setProfile(const QString &AProfile) =0;
  virtual bool renameProfile(const QString &AProfileFrom, const QString &AProfileTo) =0;
  virtual bool removeProfile(const QString &AProfile) =0;
  virtual void openOptionsDialog(const QString &ANode = "") =0;
  virtual void openOptionsNode(const QString &ANode, const QString &AName, 
    const QString &ADescription, const QIcon &AIcon) =0;
  virtual void closeOptionsNode(const QString &ANode) =0;
  virtual void appendOptionsHolder(IOptionsHolder *) =0;
  virtual void removeOptionsHolder(IOptionsHolder *) =0;
public slots:
  virtual void openOptionsDialogAction(bool) =0;
signals:
  virtual void profileAdded(const QString &AProfile) =0;
  virtual void profileOpened(const QString &AProfile) =0;
  virtual void profileClosed(const QString &AProfile) =0;
  virtual void profileRenamed(const QString &AProfileFrom, const QString &AProfileTo) =0;
  virtual void profileRemoved(const QString &AProfile) =0;
  virtual void optionsNodeOpened(const QString &ANode) =0;
  virtual void optionsNodeClosed(const QString &ANode) =0;
  virtual void optionsHolderAdded(IOptionsHolder *) =0;
  virtual void optionsHolderRemoved(IOptionsHolder *) =0;
  virtual void optionsDialogOpened() =0;
  virtual void optionsDialogAccepted() =0;
  virtual void optionsDialogRejected() =0;
  virtual void optionsDialogClosed() =0;
};

Q_DECLARE_INTERFACE(IOptionsHolder,"Vacuum.Plugin.IOptionsHolder/1.0")
Q_DECLARE_INTERFACE(ISettings,"Vacuum.Plugin.ISettings/1.0")
Q_DECLARE_INTERFACE(ISettingsPlugin,"Vacuum.Plugin.ISettingsPlugin/1.0")

#endif
