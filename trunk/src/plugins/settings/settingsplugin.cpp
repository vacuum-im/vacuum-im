#include <QtDebug>
#include "settingsplugin.h"

#include <QByteArray>
#include <QVBoxLayout>

#define DIR_ROOT                ".vacuum"
#define DIR_PROFILES            "profiles"

#define PROFILE_VERSION         "1.0"
#define SETTINGS_VERSION        "1.0"
#define DEFAULT_PROFILE         "Default"

#define ADR_PROFILE             Action::DR_Parametr1

SettingsPlugin::SettingsPlugin()
{
  FOpenOptionsDialogAction = NULL;
  FOpenProfileDialogAction = NULL;
  FTrayManager = NULL;
  FProfileMenu = NULL;
  FProfileOpened = false;
}

SettingsPlugin::~SettingsPlugin()
{
  onPluginManagerQuit();
  qDeleteAll(FNodes);
  qDeleteAll(FPluginSettings);
  delete FProfileMenu;
}

void SettingsPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing profiles and settings");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Settings Manager"); 
  APluginInfo->uid = SETTINGS_UUID;
  APluginInfo ->version = "0.1";
}

bool SettingsPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;
  connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onPluginManagerQuit()));
  
  IPlugin *plugin = FPluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  return true;
}

bool SettingsPlugin::initObjects()
{
  FProfileMenu = new Menu;
  FProfileMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_SETTINGS_PROFILES);
  FProfileMenu->setTitle(tr("Profiles"));

  FOpenOptionsDialogAction = new Action(this);
  FOpenOptionsDialogAction->setEnabled(false);
  FOpenOptionsDialogAction->setIcon(RSR_STORAGE_MENUICONS,MNI_SETTINGS_OPTIONS);
  FOpenOptionsDialogAction->setText(tr("Options..."));
  connect(FOpenOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(onOpenOptionsDialogByAction(bool)));

  FOpenProfileDialogAction = new Action(FProfileMenu);
  FOpenProfileDialogAction->setIcon(RSR_STORAGE_MENUICONS,MNI_SETTINGS_EDIT_PROFILES);
  FOpenProfileDialogAction->setText(tr("Edit profiles..."));
  FProfileMenu->addAction(FOpenProfileDialogAction,AG_DEFAULT+1);
  connect(FOpenProfileDialogAction,SIGNAL(triggered(bool)),SLOT(onOpenProfileDialogByAction(bool)));

  if (FMainWindowPlugin)
  {
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FOpenOptionsDialogAction,AG_MMENU_SETTINGS,true);
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FProfileMenu->menuAction(),AG_MMENU_SETTINGS,true);
  }

  if (FTrayManager)
  {
    FTrayManager->addAction(FOpenOptionsDialogAction,AG_TMTM_SETTINGS,true);
    FTrayManager->addAction(FProfileMenu->menuAction(),AG_TMTM_SETTINGS,true);
  }

  insertOptionsHolder(this);
  openOptionsNode(ON_MISC,tr("Misc"),tr("Extra options"),MNI_SETTINGS_OPTIONS,ONO_MISC);

  QStringList args = qApp->arguments();
  int homeDirIndex = args.indexOf(CLO_HOME_DIR);
  FHomeDir = homeDirIndex >0 ? QDir(args.value(homeDirIndex+1)) : QDir::home();
  if (!FHomeDir.exists(DIR_ROOT))
    FHomeDir.mkdir(DIR_ROOT);

  return FHomeDir.cd(DIR_ROOT);
}

bool SettingsPlugin::initSettings()
{
  if (!FHomeDir.exists(DIR_PROFILES))
    FHomeDir.mkdir(DIR_PROFILES);

  QFile profilesFile(FHomeDir.filePath(DIR_PROFILES"/profiles.xml"));
  if (!profilesFile.exists())
  {
    profilesFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
    profilesFile.close();
  }
  
  if (profilesFile.open(QIODevice::ReadOnly))
  {
    if (!FProfiles.setContent(profilesFile.readAll(),true) || FProfiles.firstChildElement().tagName() != "profiles")
    {
      FProfiles.clear();
      QDomElement elem = FProfiles.appendChild(FProfiles.createElement("profiles")).toElement();
      elem.setAttribute("version",PROFILE_VERSION);
      elem.setAttribute("profileName",DEFAULT_PROFILE);
    }
    profilesFile.close();

    foreach(QString profileName, profiles()) {
      addProfileAction(profileName); }

    if (profiles().count() == 0)
      addProfile(DEFAULT_PROFILE);

    QStringList args = qApp->arguments();
    int profileIndex = args.indexOf(CLO_PROFILE);
    if (profileIndex>0 && profiles().contains(args.value(profileIndex+1)))
      setProfile(args.value(profileIndex+1));
    else
      setProfile(FProfiles.documentElement().attribute("profileName",DEFAULT_PROFILE));
  }
  else
    qDebug() << "CANT INITIALIZE SETTINGS";

  return true;
}

//IOptionsHolder
QWidget *SettingsPlugin::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MISC)
  {
    AOrder = OWO_MISC;
    MiscOptionsWidget *widget = new MiscOptionsWidget;
    connect(this,SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(widget,SIGNAL(applied()),this,SIGNAL(optionsAccepted()));
    connect(this,SIGNAL(optionsDialogRejected()),this,SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

//ISettingsPlugin
bool SettingsPlugin::addProfile(const QString &AProfile)
{
  if (isProfilesValid())
  {
    if (!profiles().contains(AProfile))
    {
      QByteArray profileDirName = QFile::encodeName(AProfile);
      QDir profileDir = FHomeDir;
      profileDir.cd(DIR_PROFILES);
      if (!profileDir.exists(profileDirName))
      {
        if (profileDir.mkdir(profileDirName))
        {
          QDomElement profileElem = FProfiles.documentElement().appendChild(FProfiles.createElement("profile")).toElement();
          profileElem.setAttribute("name",AProfile);
          profileElem.setAttribute("dir",AProfile);
          addProfileAction(AProfile);
          emit profileAdded(AProfile);
          return true;
        }
        else
          qDebug() << "PROFILE NOT CREATED: Cant create profile directory.";
      }
      else
        qDebug() << "PROFILE NOT CREATED: Profile directory already exists.";
    }
    else
      qDebug() << "PROFILE NOT CREATED: Profile already exists.";
  }
  return false;
}

QStringList SettingsPlugin::profiles() const
{
  QStringList profileList;
  QDomElement profileElem = FProfiles.firstChildElement().firstChildElement("profile");
  while (!profileElem.isNull())
  {
    profileList.append(profileElem.attribute("name"));
    profileElem = profileElem.nextSiblingElement("profile");
  }
  return profileList;
}

QDomElement SettingsPlugin::profileNode(const QString &AProfile) 
{
  QDomElement profileElem = FProfiles.documentElement().firstChildElement("profile");  
  while (!profileElem.isNull() && profileElem.attribute("name") != AProfile)
    profileElem = profileElem.nextSiblingElement("profile"); 

  return profileElem;
}

QDomElement SettingsPlugin::pluginNode(const QUuid &APluginId) 
{
  if (isProfileOpened())
  {
    QDomElement node =  FSettings.documentElement().firstChildElement("plugin");  
    while (!node.isNull() && node.attribute("pluginId") != APluginId)
      node = node.nextSiblingElement("plugin");

    if (node.isNull())
    {
      node = FSettings.documentElement().appendChild(FSettings.createElement("plugin")).toElement();
      node.setAttribute("pluginId",APluginId.toString()); 
    }
    return node;
  }
  return QDomElement();
}

bool SettingsPlugin::setProfile(const QString &AProfile)
{
  if (profiles().contains(AProfile))
  {
    setProfileClosed();

    QDomElement profileElem = profileNode(AProfile);

    QByteArray profileDirName = QFile::encodeName(profileElem.attribute("dir",AProfile));
    FProfileDir = FHomeDir;
    if (FProfileDir.cd(DIR_PROFILES"/"+profileDirName))
    {
      QFile settingsFile(FProfileDir.filePath("settings.xml"));
      if (!settingsFile.exists())
      {
        settingsFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
        settingsFile.close();
      }
      if (settingsFile.open(QIODevice::ReadOnly))
      {
        if (!FSettings.setContent(settingsFile.readAll(),true))
        {
          FSettings.clear();
          FSettings.appendChild(FSettings.createElement("settings")).toElement().setAttribute("version",SETTINGS_VERSION);
        }
        settingsFile.close();

        FProfile = profileNode(AProfile);
        if (!FProfile.isNull() && !FSettings.isNull())
        {
          FProfiles.documentElement().setAttribute("profileName",AProfile);
          setActiveProfileAction(AProfile);
          setProfileOpened();
          return true;
        }
        else
          qDebug() << "PROFILE NOT OPENED: Profile or settings node is null.";
      }
      else
        qDebug() << "PROFILE NOT OPENED: Cant open settings file.";
    }
    else
      qDebug() << "PROFILE NOT OPENED: Profile directory not exists.";
  }
  return false;
}

bool SettingsPlugin::renameProfile(const QString &AProfileFrom, const QString &AProfileTo)
{
  QDomElement profileElem = profileNode(AProfileFrom);
  if (!profileElem.isNull() && !profiles().contains(AProfileTo))
  {
    if (AProfileFrom == profile())
      FProfiles.documentElement().setAttribute("profileName",AProfileTo);

    profileElem.setAttribute("name",AProfileTo);

    QDir profilesDir = FHomeDir;
    profilesDir.cd(DIR_PROFILES);
    if (profilesDir.rename(QFile::encodeName(profileElem.attribute("dir")),QFile::encodeName(AProfileTo)))
      profileElem.setAttribute("dir",AProfileTo);
    else
      qDebug() << "CANT RENAME PROFILE DIRECTORY";

    renameProfileAction(AProfileFrom,AProfileTo);
    emit profileRenamed(AProfileFrom,AProfileTo);
    return true;
  }
  else
    qDebug() << "CANT RENAME PROFILE: ProfileFrom not exists or ProfileTo already exists.";
  return false;
}

bool SettingsPlugin::removeProfile(const QString &AProfile)
{
  QDomElement profileElem = profileNode(AProfile);
  if (!profileElem.isNull())
  {
    if (AProfile == profile())
    {
      setProfileClosed();
      FProfiles.documentElement().setAttribute("profileName",profiles().value(0));
    }
    FProfiles.documentElement().removeChild(profileElem);

    QDir profilesDir = FHomeDir;
    profilesDir.cd(DIR_PROFILES);
    if (!profilesDir.rmdir(QFile::encodeName(profileElem.attribute("dir"))))
      qDebug() << "CANT REMOVE PROFILE DIRECTORY.";

    removeProfileAction(AProfile);
    emit profileRemoved(AProfile);
    return true;
  }
  return false;
}

bool SettingsPlugin::saveSettings()
{
  bool saved = false;
  if (isProfilesValid())
  {
    QFile profilesFile(FHomeDir.filePath(DIR_PROFILES"/profiles.xml"));
    if (profilesFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      profilesFile.write(FProfiles.toByteArray());
      profilesFile.close();
      saved = true;
    }
    else
      qDebug() << "CANT SAVE PROFILES DOCUMENT.";

    if (isProfileOpened())
    {
      QDir settingsDir = FHomeDir;
      settingsDir.cd(DIR_PROFILES);
      settingsDir.cd(QFile::encodeName(FProfile.attribute("dir")));

      QFile settingsFile(settingsDir.filePath("settings.xml"));
      if (settingsFile.exists() && settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
        settingsFile.write(FSettings.toByteArray());
        settingsFile.close();
        saved = saved && true;
      }
    }
  }
  return saved;
}

ISettings *SettingsPlugin::settingsForPlugin(const QUuid &APluginId)
{
  Settings *settings;
  if (!FPluginSettings.contains(APluginId))
  {
    settings = new Settings(APluginId,this);
    FPluginSettings.insert(APluginId,settings);
  }
  else
    settings = FPluginSettings.value(APluginId);

  return settings;
}

void SettingsPlugin::insertOptionsHolder(IOptionsHolder *AOptionsHolder)
{
  if (!FOptionsHolders.contains(AOptionsHolder))
    FOptionsHolders.append(AOptionsHolder);
}

void SettingsPlugin::removeOptionsHolder(IOptionsHolder *AOptionsHolder)
{
  if (FOptionsHolders.contains(AOptionsHolder))
    FOptionsHolders.removeAt(FOptionsHolders.indexOf(AOptionsHolder));
}

void SettingsPlugin::openOptionsNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIconKey, int AOrder)
{
  OptionsNode *node = FNodes.value(ANode,NULL);
  if (!node)
  {
    node = new OptionsNode;
    node->name = AName;
    node->desc = ADescription;
    node->icon = AIconKey;
    node->order = AOrder;
    FNodes.insert(ANode,node);
    if (!FOptionsDialog.isNull())
      FOptionsDialog->openNode(ANode,AName,ADescription,AIconKey,AOrder);
    emit optionsNodeOpened(ANode);
  }
  else
  {
    if (!AName.isEmpty())
      node->name = AName;
    if (!ADescription.isEmpty())
      node->desc = ADescription;
    if (!AIconKey.isEmpty())
      node->icon = AIconKey;
    node->order = AOrder;
  }
}

void SettingsPlugin::closeOptionsNode(const QString &ANode)
{
  OptionsNode *node = FNodes.value(ANode,NULL);
  if (node)
  {
    emit optionsNodeClosed(ANode);
    if (!FOptionsDialog.isNull())
      FOptionsDialog->closeNode(ANode);
    FNodes.remove(ANode);
    delete node;
  }
}

QDialog *SettingsPlugin::openOptionsDialog(const QString &ANode, QWidget *AParent)
{
  if (FOptionsDialog.isNull())
  {
    FOptionsDialog = new OptionsDialog(this,AParent);
    connect(FOptionsDialog, SIGNAL(accepted()),SLOT(onOptionsDialogAccepted()));
    connect(FOptionsDialog, SIGNAL(rejected()),SLOT(onOptionsDialogRejected()));
    connect(FOptionsDialog, SIGNAL(closed()),SLOT(onOptionsDialogClosed()));

    QMap<QString, OptionsNode *>::const_iterator it = FNodes.constBegin();
    while (it != FNodes.constEnd())
    {
      FOptionsDialog->openNode(it.key(),it.value()->name,it.value()->desc,it.value()->icon,it.value()->order);
      it++;
    }
    emit optionsDialogOpened();
  }
  FOptionsDialog->show();
  FOptionsDialog->showNode(ANode);
  return FOptionsDialog;
}

QWidget *SettingsPlugin::createNodeWidget(const QString &ANode)
{
  QWidget *nodeWidget = new QWidget;
  QVBoxLayout *nodeLayout = new QVBoxLayout;
  nodeLayout->setMargin(6);
  nodeLayout->setSpacing(3);
  nodeWidget->setLayout(nodeLayout);
  
  QMultiMap<int, QWidget *> widgetsByOrder;
  foreach(IOptionsHolder *optionsHolder,FOptionsHolders)
  {
    int order = 500;
    QWidget *itemWidget = optionsHolder->optionsWidget(ANode,order);
    if (itemWidget)
      widgetsByOrder.insertMulti(order,itemWidget);
  }
  
  foreach(QWidget *itemWidget,widgetsByOrder)
    nodeLayout->addWidget(itemWidget);
  
  return nodeWidget;
}

void SettingsPlugin::setProfileOpened()
{
  if (!FProfileOpened)
  {
    FOpenOptionsDialogAction->setEnabled(true);
    FProfileOpened = true;
    updateSettings();
    emit settingsOpened();
    emit profileOpened(profile());
  }
}

void SettingsPlugin::setProfileClosed()
{
  if (FProfileOpened)
  {
    if (!FOptionsDialog.isNull())
      FOptionsDialog->reject();
    emit profileClosed(profile());
    emit settingsClosed();
    saveSettings();
    FProfileOpened = false;
    FSettings.clear();
    updateSettings();
    FOpenOptionsDialogAction->setEnabled(false);
  }
}

void SettingsPlugin::updateSettings()
{
  foreach (Settings *settings, FPluginSettings)
    settings->updatePluginNode();
}

void SettingsPlugin::addProfileAction(const QString &AProfile)
{
  Action *action = new Action(FProfileMenu);
  action->setIcon(RSR_STORAGE_MENUICONS,MNI_SETTINGS_PROFILE);
  action->setText(AProfile);
  action->setCheckable(true);
  action->setData(ADR_PROFILE,AProfile);
  FProfileMenu->addAction(action,AG_DEFAULT,true);
  connect(action,SIGNAL(triggered(bool)),SLOT(onSetProfileByAction(bool)));
}

void SettingsPlugin::setActiveProfileAction(const QString &AProfile)
{
  foreach(Action *action, FProfileMenu->groupActions(AG_DEFAULT))
  {
    if (action->data(ADR_PROFILE).toString() == AProfile)
      action->setChecked(true);
    else
      action->setChecked(false);
  }
}

void SettingsPlugin::renameProfileAction(const QString &AProfileFrom, const QString &AProfileTo)
{
  QMultiHash<int,QVariant> data;
  data.insert(ADR_PROFILE,AProfileFrom);
  Action *action = FProfileMenu->findActions(data,false).value(0);
  if (action)
  {
    action->setText(AProfileTo);
    action->setData(ADR_PROFILE,AProfileTo);
  }
}

void SettingsPlugin::removeProfileAction(const QString &AProfile)
{
  QMultiHash<int,QVariant> data;
  data.insert(ADR_PROFILE,AProfile);
  Action *action = FProfileMenu->findActions(data,false).value(0);
  delete action;
}

void SettingsPlugin::onOpenProfileDialogByAction(bool)
{
  if (FProfileDialog.isNull())
    FProfileDialog = new ProfileDialog(this);
  FProfileDialog->show();
}

void SettingsPlugin::onOpenOptionsDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString node = action->data(Action::DR_Parametr1).toString();
    openOptionsDialog(node);
  }
}

void SettingsPlugin::onSetProfileByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString profileName = action->data(ADR_PROFILE).toString();
    setProfile(profileName);
  }
}

void SettingsPlugin::onOptionsDialogAccepted()
{
  emit optionsDialogAccepted();
  saveSettings();
}

void SettingsPlugin::onOptionsDialogRejected()
{
  emit optionsDialogRejected();
}

void SettingsPlugin::onOptionsDialogClosed()
{
  emit optionsDialogClosed();
}

void SettingsPlugin::onPluginManagerQuit()
{
  setProfileClosed();
}

Q_EXPORT_PLUGIN2(SettingsPlugin, SettingsPlugin)
