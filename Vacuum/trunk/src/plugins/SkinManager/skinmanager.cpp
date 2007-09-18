#include "skinmanager.h"

#include <QDir>
#include <QSet>

#define SVN_SKINSDIRECTORY            "directory"
#define SVN_SKINNAME                  "skin"

#define ADR_SKINNAME                  Action::DR_Parametr1

#define IN_APPEARANCE                 "psi/appearance"
#define IN_EYE                        "psi/eye"

SkinManager::SkinManager()
{
  FMainWindowPlugin = NULL;
  FTrayManager = NULL;
  FSettingsPlugin = NULL;
}

SkinManager::~SkinManager()
{
  if (FSkinMenu)
    delete FSkinMenu;
}

void SkinManager::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing skins");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Skin Manager"); 
  APluginInfo->uid = SKINMANAGER_UUID;
  APluginInfo ->version = "0.1";
}

bool SkinManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_SKINMANAGER;

  IPlugin *plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return true;
}

bool SkinManager::initObjects()
{
  FSkinMenu = new Menu;
  FSkinMenu->setTitle(tr("Skin"));
  FSkinMenu->setIcon(SYSTEM_ICONSETFILE,IN_APPEARANCE);
  FSkinMenu->menuAction()->setVisible(false);

  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FSkinMenu->menuAction(),AG_SKINMANAGER_MENU,true);
  }

  if (FTrayManager)
    FTrayManager->addAction(FSkinMenu->menuAction(),AG_SKINMANAGER_TRAY,true);

  return true;
}

bool SkinManager::startPlugin()
{
  if (FSkinMenu->isEmpty())
    updateSkinMenu();
  return true;
}

void SkinManager::setSkin(const QString &ASkin)
{
  emit skinAboutToBeChanged();
  Skin::setSkin(ASkin);
  updateSkinMenu();
  emit skinChanged();
}

void SkinManager::setSkinsDirectory(const QString &ASkinsDirectory)
{
  Skin::setSkinsDirectory(ASkinsDirectory);
  updateSkinMenu();
  emit skinDirectoryChanged();
}

void SkinManager::setSkinByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString skin = action->data(ADR_SKINNAME).toString();
    if (!skin.isEmpty())
      setSkin(skin);
  }
}

void SkinManager::updateSkinMenu()
{
  QSet<Action *> oldActions = FSkinMenu->actions(AG_DEFAULT).toSet();

  QHash<int,QVariant> data;
  QStringList skinList = Skin::skins();
  foreach (QString skin, skinList)
  {
    data.insert(ADR_SKINNAME,skin);
    Action *action = FSkinMenu->findActions(data,false).value(0);
    if (!action)
    {
      action = new Action(FSkinMenu);
      action->setText(skin);
      action->setIcon(Skin::skinIcon(skin));
      action->setCheckable(true);
      action->setData(ADR_SKINNAME,skin);
      connect(action,SIGNAL(triggered(bool)),SLOT(setSkinByAction(bool)));
      FSkinMenu->addAction(action,AG_DEFAULT,true);
    }
    else
      oldActions-=action;

    if (FSkinMenu->actionGroup(action) == AG_DEFAULT)
      skin == Skin::skin() ? action->setChecked(true) : action->setChecked(false);
  }

  foreach(Action *action,oldActions)
  {
    FSkinMenu->removeAction(action);
    delete action;
  }

  FSkinMenu->isEmpty() ? FSkinMenu->menuAction()->setVisible(false) : FSkinMenu->menuAction()->setVisible(true);
}

void SkinManager::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(SKINMANAGER_UUID);
  setSkinsDirectory(settings->value(SVN_SKINSDIRECTORY,Skin::skinsDirectory()).toString());
  setSkin(settings->value(SVN_SKINNAME,Skin::skin()).toString());
}

void SkinManager::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(SKINMANAGER_UUID);
  settings->setValue(SVN_SKINSDIRECTORY,Skin::skinsDirectory());
  settings->setValue(SVN_SKINNAME,Skin::skin());
}

Q_EXPORT_PLUGIN2(SkinManagerPlugin, SkinManager)
