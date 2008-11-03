#include <QtDebug>
#include "emoticons.h"
#include <QSet>

#define SMILEY_TAG_NAME                         "text"
#define SMILEY_BY_ICONSET_SCHEMA                "smiley"
#define SUBFOLDER_EMOTICONS                     "emoticons"

#define SVN_ICONSETFILES_NS                     "iconsetFiles[]"

Emoticons::Emoticons()
{
  FMessenger = NULL;
  FSkinManager = NULL;
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

  plugin = APluginManager->getPlugins("ISkinManager").value(0,NULL);
  if (plugin) 
  {
    FSkinManager = qobject_cast<ISkinManager *>(plugin->instance());
    if (FSkinManager)
    {
      connect(FSkinManager->instance(),SIGNAL(skinAboutToBeChanged()),SLOT(onSkinAboutToBeChanged()));
      connect(FSkinManager->instance(),SIGNAL(skinChanged()),SLOT(onSkinChanged()));
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
  else
    insertIconset(EMOTICONS_ICONSETFILE);

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
      QUrl url = FUrlByTag.value(cursor.selectedText());
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

void Emoticons::setIconsets(const QStringList &AIconsetFiles)
{
  foreach (QString iconsetFile, FIconsets)
  {
    removeSelectIconMenu(iconsetFile);
    emit iconsetRemoved(iconsetFile);
  }
  FIconsets.clear();
  foreach (QString iconsetFile, AIconsetFiles)
  {
    FIconsets.append(iconsetFile);
    insertSelectIconMenu(iconsetFile);
    emit iconsetInserted(iconsetFile,"");
  }
  createIconsetUrls();
}

void Emoticons::insertIconset(const QString &AIconsetFile, const QString &ABefour)
{
  if (!FIconsets.contains(AIconsetFile) && Skin::getSkinIconset(AIconsetFile)->isValid())
  {
    ABefour.isEmpty() ? FIconsets.append(AIconsetFile) : FIconsets.insert(FIconsets.indexOf(ABefour),AIconsetFile);
    createIconsetUrls();
    insertSelectIconMenu(AIconsetFile);
    emit iconsetInserted(AIconsetFile,ABefour);
  }
}

void Emoticons::insertIconset(const QStringList &AIconsetFiles, const QString &ABefour)
{
  QList<QString> insertedFiles;
  foreach(QString iconsetFile, AIconsetFiles)
  {
    if (!FIconsets.contains(iconsetFile) && Skin::getSkinIconset(iconsetFile)->isValid())
    {
      FIconsets.append(iconsetFile);
      insertedFiles.append(iconsetFile);
    }
  }
  if (!insertedFiles.isEmpty())
    createIconsetUrls();
  foreach(QString iconsetFile, insertedFiles)
  {
    insertSelectIconMenu(iconsetFile);
    emit iconsetInserted(iconsetFile,ABefour);
  }
}
void Emoticons::removeIconset(const QString &AIconsetFile)
{
  if (FIconsets.contains(AIconsetFile))
  {
    FIconsets.removeAt(FIconsets.indexOf(AIconsetFile));
    removeSelectIconMenu(AIconsetFile);
    createIconsetUrls();
    emit iconsetRemoved(AIconsetFile);
  }
}

void Emoticons::removeIconset(const QStringList &AIconsetFiles)
{
  foreach(QString iconsetFile, AIconsetFiles)
  {
    if (FIconsets.contains(iconsetFile))
    {
      FIconsets.removeAt(FIconsets.indexOf(iconsetFile));
      removeSelectIconMenu(iconsetFile);
      emit iconsetRemoved(iconsetFile);
    }
  }
  createIconsetUrls();
}

QUrl Emoticons::urlByTag(const QString &ATag) const
{
  return FUrlByTag.value(ATag);
}

QString Emoticons::tagByUrl(const QUrl &AUrl) const
{
  return FUrlByTag.key(AUrl);
}

QIcon Emoticons::iconByTag(const QString &ATag) const
{
  return iconByUrl(urlByTag(ATag));
}

QIcon Emoticons::iconByUrl(const QUrl &AUrl) const
{
  QIcon icon = FIconByUrl.value(AUrl.toString());
  if (icon.isNull())
  {
    if (AUrl.scheme() == SMILEY_BY_ICONSET_SCHEMA)
    {
      QString fileName = AUrl.path();
      QString tagValue = AUrl.fragment();
      if (!fileName.isEmpty() && !tagValue.isEmpty())
      {
        SkinIconset *iconset = Skin::getSkinIconset(fileName);
        icon = iconset->iconByTagValue(SMILEY_TAG_NAME,tagValue);
        if (!icon.isNull())
          FIconByUrl.insert(AUrl.toString(),icon);
      }
    }
  }
  return icon;
}

void Emoticons::createIconsetUrls()
{
  FUrlByTag.clear();
  FIconByUrl.clear();
  foreach(QString iconsetFile, FIconsets)
  {
    SkinIconset *iconset = Skin::getSkinIconset(iconsetFile);
    QList<QString> tags = iconset->tagValues(SMILEY_TAG_NAME);
    foreach(QString tag, tags)
    {
      if (!FUrlByTag.contains(tag))
      {
        QUrl url;
        url.setScheme(SMILEY_BY_ICONSET_SCHEMA);
        url.setPath(iconsetFile);
        url.setFragment(tag);
        FUrlByTag.insert(tag,url);
      }
    }
  }
}

SelectIconMenu *Emoticons::createSelectIconMenu(const QString &AIconsetFile, QWidget *AParent)
{
  SelectIconMenu *menu = new SelectIconMenu(AParent);
  menu->setIconset(AIconsetFile);
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
    foreach(QString iconsetFile, FIconsets)
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

void Emoticons::onIconSelected(const QString &AIconsetFile, const QString &AIconFile)
{
  SelectIconMenu *menu = qobject_cast<SelectIconMenu *>(sender());
  if (FToolBarWidgetByMenu.contains(menu))
  {
    IEditWidget *widget = FToolBarWidgetByMenu.value(menu)->editWidget();
    if (widget)
    {
      SkinIconset *iconset = Skin::getSkinIconset(AIconsetFile);
      QString text = iconset->tagValues(SMILEY_TAG_NAME,AIconFile).value(0);
      widget->textEdit()->setFocus();
      widget->textEdit()->textCursor().insertText(text);
      widget->textEdit()->textCursor().insertText(" ");
    }
  }
}

void Emoticons::onSelectIconMenuDestroyed(QObject *AObject)
{
  FToolBarWidgetByMenu.remove((SelectIconMenu *)AObject);
}

void Emoticons::onSkinAboutToBeChanged()
{
  onSettingsClosed();
}

void Emoticons::onSkinChanged()
{
  onSettingsOpened();
  emit iconsetsChangedBySkin();
}

void Emoticons::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(EMOTICONS_UUID);
  QStringList smiles =  Skin::skinFiles(SKIN_TYPE_ICONSET,SUBFOLDER_EMOTICONS,"*.jisp");
  for(int i = 0; i<smiles.count(); i++)
    smiles[i].prepend(SUBFOLDER_EMOTICONS"/");
  setIconsets(settings->valueNS(SVN_ICONSETFILES_NS,Skin::skin(),smiles).toStringList());
}

void Emoticons::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(EMOTICONS_UUID);
  settings->setValueNS(SVN_ICONSETFILES_NS,Skin::skin(),FIconsets);
  removeIconset(FIconsets);
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
