#ifndef SIMPLEMESSAGESTYLEPLUGIN_H
#define SIMPLEMESSAGESTYLEPLUGIN_H

#include "../../definations/resources.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagestyles.h"
#include "../../interfaces/isettings.h"
#include "../../utils/message.h"
#include "simplemessagestyle.h"

#define SIMPLEMESSAGESTYLE_UUID   "{cfad7d10-58d0-4638-9940-dda64c1dd509}"

class SimpleMessageStylePlugin : 
  public QObject,
  public IPlugin,
  public IMessageStylePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageStylePlugin);
public:
  SimpleMessageStylePlugin();
  ~SimpleMessageStylePlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SIMPLEMESSAGESTYLE_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IMessageStylePlugin
  virtual QString stylePluginId() const;
  virtual QList<QString> styles() const;
  virtual IMessageStyle *styleById(const QString &AStyleId);
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const;
  virtual void setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext = QString::null);
signals:
  virtual void styleCreated(IMessageStyle *AStyle) const;
  virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const;
protected:
  void updateAvailStyles();
private:
  ISettingsPlugin *FSettingsPlugin;
private:
  QMap<QString, QString> FStylePaths;
  QMap<QString, IMessageStyle *> FStyles;
};

#endif // SIMPLEMESSAGESTYLEPLUGIN_H
