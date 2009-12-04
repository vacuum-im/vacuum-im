#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QHash>
#include <QPointer>
#include <QTranslator>
#include <QDomDocument>
#include <QApplication>
#include <QPluginLoader>
#include <definations/config.h>
#include <definations/plugininitorders.h>
#include <definations/commandline.h>
#include <definations/actiongroups.h>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <definations/version.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/action.h>
#include "setuppluginsdialog.h"
#include "aboutbox.h"

struct PluginItem 
{
  IPlugin *plugin;
  IPluginInfo *info;
  QPluginLoader *loader;
  QTranslator *translator;
};

class PluginManager : 
  public QObject, 
  public IPluginManager
{
  Q_OBJECT;
  Q_INTERFACES(IPluginManager);
public:
  PluginManager(QApplication *AParent);
  ~PluginManager();
  virtual QObject *instance() { return this; }
  virtual QString version() const;
  virtual int revision() const;
  virtual QDateTime revisionDate() const;
  virtual QString homePath() const;
  virtual void setHomePath(const QString &APath);
  virtual void setLocale(QLocale::Language ALanguage, QLocale::Country ACountry);
  virtual IPlugin* pluginInstance(const QUuid &AUuid) const;
  virtual QList<IPlugin *> pluginInterface(const QString &AInterface = QString::null) const;
  virtual const IPluginInfo *pluginInfo(const QUuid &AUuid) const;
  virtual QList<QUuid> pluginDependencesOn(const QUuid &AUuid) const;
  virtual QList<QUuid> pluginDependencesFor(const QUuid &AUuid) const;
public slots:
  virtual void quit();
  virtual void restart();
signals:
  virtual void aboutToQuit();
protected:
  void loadSettings();
  void saveSettings();
  void loadPlugins();
  void initPlugins();
  void startPlugins();
protected:
  void removePluginItem(const QUuid &AUuid, const QString &AError);
  void unloadPlugin(const QUuid &AUuid, const QString &AError = QString::null);
  bool checkDependences(const QUuid AUuid) const;
  bool checkConflicts(const QUuid AUuid) const;
  QList<QUuid> getConflicts(const QUuid AUuid) const;
  void loadCoreTranslations(const QString &ADir);
protected:
  bool isPluginEnabled(const QString &AFile) const;
  QDomElement savePluginInfo(const QString &AFile, const IPluginInfo *AInfo);
  void savePluginError(const QString &AFile, const QString &AEror);
  void removePluginsInfo(const QStringList &ACurFiles);
  void createMenuActions();
protected slots:
  void onApplicationAboutToQuit();
  void onShowSetupPluginsDialog(bool);
  void onShowAboutBoxDialog();
private:
  QPointer<AboutBox> FAboutDialog;
  QPointer<SetupPluginsDialog> FPluginsDialog;
private:
  QString FHomePath;
  QDomDocument FPluginsSetup;
  QTranslator *FQtTranslator;
  QTranslator *FUtilsTranslator;
  QTranslator *FLoaderTranslator;
  QHash<QUuid, PluginItem> FPluginItems;
  mutable QMultiHash<QString, IPlugin *> FPlugins;
};

#endif
