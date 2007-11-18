#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QPointer>
#include "../../definations/actiongroups.h"
#include "../../definations/messagewriterorders.h"
#include "../../definations/resourceloaderorders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iemoticons.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/iskinmanager.h"
#include "../../interfaces/isettings.h"
#include "../../utils/skin.h"
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
  virtual void pluginInfo(PluginInfo *APluginInfo);
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
  virtual QStringList iconsets() const { return FIconsets; }
  virtual void setIconsets(const QStringList &AIconsetFiles);
  virtual void insertIconset(const QString &AIconsetFile, const QString &ABefour = "");
  virtual void insertIconset(const QStringList &AIconsetFiles, const QString &ABefour = "");
  virtual void removeIconset(const QString &AIconsetFile);
  virtual void removeIconset(const QStringList &AIconsetFiles);
  virtual QUrl urlByTag(const QString &ATag) const;
  virtual QString tagByUrl(const QUrl &AUrl) const;
  virtual QIcon iconByTag(const QString &ATag) const;
  virtual QIcon iconByUrl(const QUrl &AUrl) const;
signals:
  virtual void iconsetInserted(const QString &AIconsetFile, const QString &ABefour);
  virtual void iconsetRemoved(const QString &AIconsetFile);
  virtual void iconsetsChangedBySkin();
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void createIconsetUrls();
  SelectIconMenu *createSelectIconMenu(const QString &AIconsetFile, QWidget *AParent);
  void insertSelectIconMenu(const QString &AIconsetFile);
  void removeSelectIconMenu(const QString &AIconsetFile);
protected slots:
  void onToolBarWidgetCreated(IToolBarWidget *AWidget);
  void onToolBarWidgetDestroyed(QObject *AObject);
  void onIconSelected(const QString &AIconsetFile, const QString &AIconFile);
  void onSelectIconMenuDestroyed(QObject *AObject);
  void onSkinAboutToBeChanged();
  void onSkinChanged();
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
private:
  IMessenger *FMessenger;
  ISkinManager *FSkinManager;
  ISettingsPlugin *FSettingsPlugin;
private:
  QPointer<EmoticonsOptions> FEmoticonsOptions;
private:
  QList<IToolBarWidget *> FToolBarsWidgets;
  QHash<SelectIconMenu *, IToolBarWidget *> FToolBarWidgetByMenu;
  QStringList FIconsets;
  QHash<QString,QUrl> FUrlByTag;
  mutable QHash<QString,QIcon> FIconByUrl;
};

#endif // EMOTICONS_H
