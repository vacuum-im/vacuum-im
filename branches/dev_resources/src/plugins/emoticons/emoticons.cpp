#include <QtDebug>
#include "emoticons.h"
#include <QSet>

#define SMILEY_BY_ICONSET_SCHEMA        "smiley"
#define SVN_SUBSTORAGES                 "substorages"

Emoticons::Emoticons()
{
  FMessenger = NULL;
  FSettingsPlugin = NULL;
}

Emoticons::~Emoticons()
{

}

void Emoticons::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Insert smiley icons in chat windows");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Emoticons"); 
  APluginInfo->uid = EMOTICONS_UUID;
  APluginInfo->version = "0.1";
}

bool Emoticons::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IMessenger").value(0,NULL);
  if (plugin)
  {
    FMessenger = qobject_cast<IMessenger *>(plugin->instance());
    if (FMessenger)
    {
      connect(FMessenger->instance(),SIGNAL(toolBarWidgetCreated(IToolBarWidget *)),SLOT(onToolBarWidgetCreated(IToolBarWidget *)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin) 
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
    }
  }

  return true;
}

bool Emoticons::initObjects()
{
  if (FMessenger)
  {
    FMessenger->insertMessageWriter(this,MWO_EMOTICONS);
    FMessenger->insertResourceLoader(this,RSLO_EMOTICONS);
  }

  if (FSettingsPlugin != NULL)
  {
    FSettingsPlugin->openOptionsNode(ON_EMOTICONS ,tr("Emoticons"), tr("Select emoticons files"),QIcon());
    FSettingsPlugin->insertOptionsHolder(this);
  }

  return true;
}

void Emoticons::writeMessage(Message &/*AMessage*/, QTextDocument *ADocument, const QString &/*ALang*/, int AOrder)
{
  static QChar imageChar = QChar::ObjectReplacementCharacter;
  if (AOrder == MWO_EMOTICONS)
  {
    for (QTextCursor cursor = ADocument->find(imageChar); !cursor.isNull();  cursor = ADocument->find(imageChar,cursor))
    {
      QUrl imageUrl = cursor.charFormat().toImageFormat().name();
      if (imageUrl.scheme() == SMILEY_BY_ICONSET_SCHEMA)
      {
        cursor.insertText(imageUrl.fragment());
        cursor.insertText(" ");
      }
    }
  }
}

void Emoticons::writeText(Message &/*AMessage*/, QTextDocument *ADocument, const QString &/*ALang*/, int AOrder)
{
  static QChar blankChar = QChar::Nbsp;
  if (AOrder == MWO_EMOTICONS)
  {
    QRegExp regexp("\\S+");
    for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
    {
      QUrl url = FUrlByKey.value(cursor.selectedText());
      if (!url.isEmpty())
        cursor.insertImage(url.toString());
    }
  }
}

void Emoticons::loadResource(int AType, const QUrl &AName, QVariant &AValue)
{
  if (AType == QTextDocument::ImageResource)
  {
    QIcon icon = iconByUrl(AName);
    if (!icon.isNull())
      AValue = icon.pixmap(QSize(16,16));
  }
}

QWidget *Emoticons::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_EMOTICONS)
  {
    AOrder = OO_EMOTICONS;
    FEmoticonsOptions = new EmoticonsOptions(this);
    return FEmoticonsOptions;
  }
  return NULL;
}

void Emoticons::setIconsets(const QStringList &ASubStorages)
{
  QSet<QString> removeList = FStoragesOrder.toSet() - ASubStorages.toSet();
  foreach (QString substorage, removeList)
  {
    removeSelectIconMenu(substorage);
    FStoragesOrder.removeAt(FStoragesOrder.indexOf(substorage));
    delete FStorages.take(substorage);
    emit iconsetRemoved(substorage);
  }

  foreach (QString substorage, ASubStorages)
  {
    if (!FStoragesOrder.contains(substorage))
    {
      FStoragesOrder.append(substorage);
      FStorages.insert(substorage,new IconStorage(RSR_STORAGE_EMOTICONS,substorage,this));
      insertSelectIconMenu(substorage);
      emit iconsetInserted(substorage,"");
    }
  }
  FStoragesOrder = ASubStorages;
  createIconsetUrls();
}

void Emoticons::insertIconset(const QString &ASubStorage, const QString &ABefour)
{
  if (!FStoragesOrder.contains(ASubStorage))
  {
    ABefour.isEmpty() ? FStoragesOrder.append(ASubStorage) : FStoragesOrder.insert(FStoragesOrder.indexOf(ABefour),ASubStorage);
    FStorages.insert(ASubStorage,new IconStorage(RSR_STORAGE_EMOTICONS,ASubStorage,this));
    insertSelectIconMenu(ASubStorage);
    createIconsetUrls();
    emit iconsetInserted(ASubStorage,ABefour);
  }
}

void Emoticons::removeIconset(const QString &ASubStorage)
{
  if (FStoragesOrder.contains(ASubStorage))
  {
    removeSelectIconMenu(ASubStorage);
    FStoragesOrder.removeAt(FStoragesOrder.indexOf(ASubStorage));
    delete FStorages.take(ASubStorage);
    createIconsetUrls();
    emit iconsetRemoved(ASubStorage);
  }
}

QUrl Emoticons::urlByKey(const QString &AKey) const
{
  return FUrlByKey.value(AKey);
}

QString Emoticons::keyByUrl(const QUrl &AUrl) const
{
  return FUrlByKey.key(AUrl);
}

QIcon Emoticons::iconByKey(const QString &AKey) const
{
  return iconByUrl(urlByKey(AKey));
}

QIcon Emoticons::iconByUrl(const QUrl &AUrl) const
{
  if (AUrl.scheme() == SMILEY_BY_ICONSET_SCHEMA)
  {
    QString key = AUrl.fragment();
    QString substorage = AUrl.path();
    if (!key.isEmpty() && FStorages.contains(substorage))
      return FStorages.value(substorage)->getIcon(key);
  }
  return QIcon();
}

void Emoticons::createIconsetUrls()
{
  FUrlByKey.clear();
  foreach(QString substorage, FStoragesOrder)
  {
    IconStorage *storage = FStorages.value(substorage);
    foreach(QString key, storage->fileKeys())
    {
      if (!FUrlByKey.contains(key))
      {
        QUrl url;
        url.setScheme(SMILEY_BY_ICONSET_SCHEMA);
        url.setPath(substorage);
        url.setFragment(key);
        FUrlByKey.insert(key,url);
      }
    }
  }
}

SelectIconMenu *Emoticons::createSelectIconMenu(const QString &ASubStorage, QWidget *AParent)
{
  SelectIconMenu *menu = new SelectIconMenu(AParent);
  menu->setIconset(ASubStorage);
  connect(menu->instance(),SIGNAL(iconSelected(const QString &, const QString &)),
    SLOT(onIconSelected(const QString &, const QString &)));
  connect(menu->instance(),SIGNAL(destroyed(QObject *)),SLOT(onSelectIconMenuDestroyed(QObject *)));
  return menu;
}

void Emoticons::insertSelectIconMenu(const QString &AIconsetFile)
{
  foreach(IToolBarWidget *widget, FToolBarsWidgets)
  {
    SelectIconMenu *menu = createSelectIconMenu(AIconsetFile,widget);
    FToolBarWidgetByMenu.insert(menu,widget);
    QToolButton *button = widget->toolBarChanger()->addToolButton(menu->menuAction(),AG_EMOTICONS_EDITWIDGET,false);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setPopupMode(QToolButton::InstantPopup);
  }
}

void Emoticons::removeSelectIconMenu(const QString &AIconsetFile)
{
  QHash<SelectIconMenu *,IToolBarWidget *>::iterator it = FToolBarWidgetByMenu.begin();
  while (it != FToolBarWidgetByMenu.end())
  {
    SelectIconMenu *menu = it.key();
    IToolBarWidget *widget = it.value();
    if (menu->iconset() == AIconsetFile)
    {
      widget->toolBarChanger()->removeAction(menu->menuAction());
      it = FToolBarWidgetByMenu.erase(it);
      delete menu;
    }
    else
      it++;
  }
}

void Emoticons::onToolBarWidgetCreated(IToolBarWidget *AWidget)
{
  if (AWidget->editWidget() != NULL)
  {
    FToolBarsWidgets.append(AWidget);
    foreach(QString iconsetFile, FStoragesOrder)
    {
      SelectIconMenu *menu = createSelectIconMenu(iconsetFile,AWidget);
      FToolBarWidgetByMenu.insert(menu,AWidget);
      QToolButton *button = AWidget->toolBarChanger()->addToolButton(menu->menuAction(),AG_EMOTICONS_EDITWIDGET);
      button->setToolButtonStyle(Qt::ToolButtonIconOnly);
      button->setPopupMode(QToolButton::InstantPopup);
    }
    connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onToolBarWidgetDestroyed(QObject *)));
  }
}

void Emoticons::onToolBarWidgetDestroyed(QObject *AObject)
{
  IToolBarWidget *widget = static_cast<IToolBarWidget *>(AObject);
  FToolBarsWidgets.removeAt(FToolBarsWidgets.indexOf(widget));
}

void Emoticons::onIconSelected(const QString &/*ASubStorage*/, const QString &AIconKey)
{
  SelectIconMenu *menu = qobject_cast<SelectIconMenu *>(sender());
  if (FToolBarWidgetByMenu.contains(menu))
  {
    IEditWidget *widget = FToolBarWidgetByMenu.value(menu)->editWidget();
    if (widget)
    {
      widget->textEdit()->setFocus();
      widget->textEdit()->textCursor().insertText(AIconKey);
      widget->textEdit()->textCursor().insertText(" ");
    }
  }
}

void Emoticons::onSelectIconMenuDestroyed(QObject *AObject)
{
  FToolBarWidgetByMenu.remove((SelectIconMenu *)AObject);
}

void Emoticons::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(EMOTICONS_UUID);
  setIconsets(settings->value(SVN_SUBSTORAGES,FileStorage::availSubStorages(RSR_STORAGE_EMOTICONS)).toStringList());
}

void Emoticons::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(EMOTICONS_UUID);
  settings->setValue(SVN_SUBSTORAGES,FStoragesOrder);
}

void Emoticons::onOptionsDialogAccepted()
{
  if (!FEmoticonsOptions.isNull())
    FEmoticonsOptions->apply();
  emit optionsAccepted();
}

void Emoticons::onOptionsDialogRejected()
{
  emit optionsRejected();
}

Q_EXPORT_PLUGIN2(EmoticonsPlugin, Emoticons)
