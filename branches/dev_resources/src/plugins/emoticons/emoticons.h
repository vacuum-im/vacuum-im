#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QPointer>
#include <QStringList>
#include "../../definations/actiongroups.h"
#include "../../definations/messagewriterorders.h"
#include "../../definations/resourceloaderorders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iemoticons.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/isettings.h"
#include "../../utils/iconstorage.h"
#include "../../utils/menu.h"
#include "selecticonwidget.h"
#include "selecticonmenu.h"
#include "emoticonsoptions.h"

class Emoticons : 
  public QObject,
  public IPlugin,
  public IEmoticons,
  public IMessageWriter,
  public IResourceLoader,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IEmoticons IMessageWriter IResourceLoader IOptionsHolder);
public:
  Emoticons();
  ~Emoticons();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return EMOTICONS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IMessageWriter
  virtual void writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  virtual void writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  //IResourceLoader
  virtual void loadResource(int AType, const QUrl &AName, QVariant &AValue);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IEmoticons
  virtual QStringList iconsets() const { return FStoragesOrder; }
  virtual void setIconsets(const QStringList &ASubStorages);
  virtual void insertIconset(const QString &ASubStorage, const QString &ABefour = "");
  virtual void removeIconset(const QString &ASubStorage);
  virtual QUrl urlByKey(const QString &AKey) const;
  virtual QString keyByUrl(const QUrl &AUrl) const;
  virtual QIcon iconByKey(const QString &AKey) const;
  virtual QIcon iconByUrl(const QUrl &AUrl) const;
signals:
  virtual void iconsetInserted(const QString &ASubStorage, const QString &ABefour);
  virtual void iconsetRemoved(const QString &ASubStorage);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void createIconsetUrls();
  SelectIconMenu *createSelectIconMenu(const QString &ASubStorage, QWidget *AParent);
  void insertSelectIconMenu(const QString &ASubStorage);
  void removeSelectIconMenu(const QString &ASubStorage);
protected slots:
  void onToolBarWidgetCreated(IToolBarWidget *AWidget);
  void onToolBarWidgetDestroyed(QObject *AObject);
  void onIconSelected(const QString &ASubStorage, const QString &AIconKey);
  void onSelectIconMenuDestroyed(QObject *AObject);
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
private:
  IMessenger *FMessenger;
  ISettingsPlugin *FSettingsPlugin;
private:
  QPointer<EmoticonsOptions> FEmoticonsOptions;
private:
  QList<IToolBarWidget *> FToolBarsWidgets;
  QHash<SelectIconMenu *, IToolBarWidget *> FToolBarWidgetByMenu;
  QStringList FStoragesOrder;
  QHash<QString, IconStorage *> FStorages;
  QHash<QString,QUrl> FUrlByKey;
};

#endif // EMOTICONS_H
