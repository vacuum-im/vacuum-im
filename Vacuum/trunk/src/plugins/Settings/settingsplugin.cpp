#include <QtDebug>
#include "settingsplugin.h"

#include <QByteArray>
#include <QVBoxLayout>

#define PROFILE_VERSION "1.0"
#define SETTINGS_VERSION "1.0"

SettingsPlugin::SettingsPlugin()
{
  FOpenOptionsDialogAction = NULL;
  FOpenProfileDialogAction = NULL;
  FTrayManager = NULL;
  FProfileOpened = false;
  FSystemIconset.openFile(SYSTEM_ICONSETFILE);
}

SettingsPlugin::~SettingsPlugin()
{
  onPluginManagerQuit();
  qDeleteAll(FNodes);
  qDeleteAll(FPluginSettings);
}

void SettingsPlugin::pluginInfo(PluginInfo *APluginInfo)
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
    if (FMainWindowPlugin)
    {
      connect(FMainWindowPlugin->instance(),SIGNAL(mainWindowCreated(IMainWindow *)),
        SLOT(onMainWindowCreated(IMainWindow *)));
    }
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
  FOpenOptionsDialogAction = new Action(this);
  FOpenOptionsDialogAction->setEnabled(false);
  FOpenOptionsDialogAction->setIcon(SYSTEM_ICONSETFILE,"psi/options");
  FOpenOptionsDialogAction->setText(tr("Options..."));
  connect(FOpenOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(openOptionsDialogByAction(bool)));

  FOpenProfileDialogAction = new Action(this);
  FOpenProfileDialogAction->setIcon(SYSTEM_ICONSETFILE,"psi/profile");
  FOpenProfileDialogAction->setText(tr("Change profile..."));
  connect(FOpenProfileDialogAction,SIGNAL(triggered(bool)),SLOT(openProfileDialogByAction(bool)));

  if (FTrayManager)
  {
    FTrayManager->addAction(FOpenOptionsDialogAction,AG_SETTINGS_TRAY,true);
    FTrayManager->addAction(FOpenProfileDialogAction,AG_SETTINGS_TRAY,true);
  }
  return true;
}

bool SettingsPlugin::initSettings()
{
  static const char *clientDirName = ".vacuum";
  
  FHomeDir = QDir::home();
  if (!FHomeDir.exists(clientDirName))
    FHomeDir.mkdir(clientDirName);

  bool settingsReady = FHomeDir.cd(clientDirName);
  if (settingsReady)
  {
    if (!FHomeDir.exists("profiles"))
      FHomeDir.mkdir("profiles");

    QFile profilesFile(FHomeDir.filePath("profiles/profiles.xml"));
    
    if (!profilesFile.exists())
    {
      profilesFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
      profilesFile.close();
    }
    
    settingsReady = profilesFile.open(QIODevice::ReadOnly);
    if (!FProfiles.setContent(profilesFile.readAll(),true) || FProfiles.firstChildElement().tagName() != "profiles")
    {
      FProfiles.clear();
      QDomElement elem = FProfiles.appendChild(FProfiles.createElement("profiles")).toElement();
      elem.setAttribute("version",PROFILE_VERSION);
      elem.setAttribute("profileName","Default");
    }
    profilesFile.close();
    
    if (settingsReady)
    {
      if (profiles().count() == 0)
        addProfile("Default");
      setProfile(FProfiles.documentElement().attribute("profileName","Default"));
    }
  }

  if (!settingsReady)
    qDebug() << "CANT INITIALIZE SETTINGS";

  return settingsReady;
}

//ISettingsPlugin
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

bool SettingsPlugin::saveSettings()
{
  bool saved = false;
  if (isProfilesValid())
  {
    QFile profilesFile(FHomeDir.filePath("profiles/profiles.xml"));
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
      settingsDir.cd("profiles");
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

bool SettingsPlugin::addProfile(const QString &AProfile)
{
  if (isProfilesValid())
  {
    if (!profiles().contains(AProfile))
    {
      QByteArray profileDirName = QFile::encodeName(AProfile);
      QDir profileDir = FHomeDir;
      profileDir.cd("profiles");
      if (!profileDir.exists(profileDirName))
      {
        if (profileDir.mkdir(profileDirName))
        {
          QDomElement profileElem = FProfiles.documentElement().appendChild(FProfiles.createElement("profile")).toElement();
          profileElem.setAttribute("name",AProfile);
          profileElem.setAttribute("dir",AProfile);
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
    if (FProfileDir.cd("profiles/"+profileDirName))
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
    profilesDir.cd("profiles");
    if (profilesDir.rename(QFile::encodeName(profileElem.attribute("dir")),QFile::encodeName(AProfileTo)))
      profileElem.setAttribute("dir",AProfileTo);
    else
      qDebug() << "CANT RENAME PROFILE DIRECTORY";
    
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
    profilesDir.cd("profiles");
    if (!profilesDir.remove(QFile::encodeName(profileElem.attribute("dir"))))
      qDebug() << "CANT REMOVE PROFILE DIRECTORY.";

    emit profileRemoved(AProfile);

    return true;
  }
  return false;
}

void SettingsPlugin::openOptionsNode(const QString &ANode, const QString &AName,  
                                     const QString &ADescription, const QIcon &AIcon)
{
  OptionsNode *node = FNodes.value(ANode,NULL);
  if (!node)
  {
    node = new OptionsNode;
    node->name = AName;
    node->desc = ADescription;
    node->icon = AIcon;
    FNodes.insert(ANode,node);
    if (!FOptionsDialog.isNull())
      FOptionsDialog->openNode(ANode,AName,ADescription,AIcon,createNodeWidget(ANode));
    emit optionsNodeOpened(ANode);
  }
  else
  {
    if (!AName.isEmpty())
      node->name = AName;
    if (!ADescription.isEmpty())
      node->desc = ADescription;
    if (!AIcon.isNull())
      node->icon = AIcon;
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

void SettingsPlugin::appendOptionsHolder(IOptionsHolder *AOptionsHolder)
{
  if (!FOptionsHolders.contains(AOptionsHolder))
  {
    FOptionsHolders.append(AOptionsHolder);
    emit optionsHolderAdded(AOptionsHolder);
  }
}

void SettingsPlugin::removeOptionsHolder(IOptionsHolder *AOptionsHolder)
{
  if (!FOptionsHolders.contains(AOptionsHolder))
  {
    emit optionsHolderRemoved(AOptionsHolder);
    FOptionsHolders.removeAt(FOptionsHolders.indexOf(AOptionsHolder));
  }
}

void SettingsPlugin::openOptionsDialog(const QString &ANode)
{
  if (FOptionsDialog.isNull())
  {
    FOptionsDialog = new OptionsDialog;
    FOptionsDialog->setAttribute(Qt::WA_DeleteOnClose,true);
    FOptionsDialog->setWindowIcon(FSystemIconset.iconByName("psi/options"));
    connect(FOptionsDialog, SIGNAL(opened()),SLOT(onOptionsDialogOpened()));
    connect(FOptionsDialog, SIGNAL(accepted()),SLOT(onOptionsDialogAccepted()));
    connect(FOptionsDialog, SIGNAL(rejected()),SLOT(onOptionsDialogRejected()));
    connect(FOptionsDialog, SIGNAL(closed()),SLOT(onOptionsDialogClosed()));

    QMap<QString, OptionsNode *>::const_iterator it = FNodes.constBegin();
    while (it != FNodes.constEnd())
    {
      FOptionsDialog->openNode(it.key(),it.value()->name,it.value()->desc,it.value()->icon,createNodeWidget(it.key()));
      it++;
    }
  }
  if (!ANode.isEmpty())
    FOptionsDialog->showNode(ANode);
  FOptionsDialog->show();
}

void SettingsPlugin::openOptionsDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString node = action->data(Action::DR_Parametr1).toString();
    openOptionsDialog(node);
  }
}

void SettingsPlugin::openProfileDialogByAction(bool)
{
  if (FProfileDialog.isNull())
    FProfileDialog = new ProfileDialog(this);
  FProfileDialog->show();
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
  nodeLayout->addStretch();
  
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

void SettingsPlugin::onMainWindowCreated(IMainWindow *AMainWindow)
{
  AMainWindow->mainMenu()->addAction(FOpenOptionsDialogAction,AG_SETTINGS_MMENU,true);
  AMainWindow->mainMenu()->addAction(FOpenProfileDialogAction,AG_SETTINGS_MMENU,true);
}

void SettingsPlugin::onOptionsDialogOpened()
{
  emit optionsDialogOpened();
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
  if (!FOptionsDialog.isNull())
    FOptionsDialog->reject();

  setProfileClosed();
}

Q_EXPORT_PLUGIN2(SettingsPlugin, SettingsPlugin)
