#include <QtDebug>
#include "settingsplugin.h"

#include <QByteArray>
#include <QVBoxLayout>
#include "../../definations/actiongroups.h"

SettingsPlugin::SettingsPlugin()
{
  actOpenOptionsDialog = NULL;
  FTrayManager = NULL;
  FProfileOpened = false;
  FFile.setParent(this);
  FSystemIconset.openFile(SYSTEM_ICONSETFILE);
}

SettingsPlugin::~SettingsPlugin()
{
  onPluginManagerQuit();
  qDeleteAll(FNodes);
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

  return setSettingsFile("config.xml");
}

bool SettingsPlugin::initObjects()
{
  actOpenOptionsDialog = new Action(this);
  actOpenOptionsDialog->setEnabled(false);
  actOpenOptionsDialog->setIcon(SYSTEM_ICONSETFILE,"psi/options");
  actOpenOptionsDialog->setText(tr("Options..."));
  connect(actOpenOptionsDialog,SIGNAL(triggered(bool)),SLOT(openOptionsDialogAction(bool)));
  if (FTrayManager)
    FTrayManager->addAction(actOpenOptionsDialog,SETTINGS_ACTION_GROUP_OPTIONS,true);
  return true;
}

bool SettingsPlugin::initSettings()
{
  setProfile();
  return true;
}

//ISettingsPlugin
ISettings *SettingsPlugin::openSettings(const QUuid &APluginId, QObject *AParent)
{
  Settings *settings = new Settings(APluginId,this,AParent);
  FCleanupHandler.add(settings); 
  return settings;
}

bool SettingsPlugin::setSettingsFile(const QString &AFileName)
{
  setProfileClosed();
  FSettings.clear();

  FFile.setFileName(AFileName);

  if (!FFile.exists())
    if (FFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
      FFile.close(); 

  if (FFile.open(QIODevice::ReadOnly))
  {
    if (FFile.size() == 0)
    {
      QDomElement config = FSettings.appendChild(FSettings.createElement("config")).toElement();
      config.setAttribute("version","1.0");
      config.setAttribute("profile","Default");
    }
    else
    {
      if (!FSettings.setContent(FFile.readAll(),true))
        FSettings.clear();
    }
    FFile.close();
  }
  return !FSettings.isNull();
}

bool SettingsPlugin::saveSettings()
{
  if (FFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    FFile.write(FSettings.toByteArray());  
    FFile.close(); 
    return true;
  }
  return false;
}

QDomElement SettingsPlugin::addProfile(const QString &AProfile)
{
  QDomElement profileElem;
  if (isSettingsValid())
  {
    if (!profiles().contains(AProfile))
    {
      profileElem = FSettings.documentElement().appendChild(FSettings.createElement("profile")).toElement();
      profileElem.setAttribute("name",AProfile); 
      emit profileAdded(AProfile);
    }
    else
      profileElem = profileNode(AProfile);
  }
  return profileElem;
}

QStringList SettingsPlugin::profiles() const
{
  QStringList profileList;
  QDomElement profileElem = FSettings.documentElement().firstChildElement("profile");
  while (!profileElem.isNull())
  {
    profileList.append(profileElem.attribute("name"));
    profileElem = profileElem.nextSiblingElement("profile");
  }
  return profileList;
}

QDomElement SettingsPlugin::profileNode(const QString &AProfile) 
{
  QString profileName = AProfile;
  if (profileName.isEmpty())
    profileName = FSettings.documentElement().attribute("profile","Default");

  QDomElement profileElem = FSettings.documentElement().firstChildElement("profile");  
  while (!profileElem.isNull() && profileElem.attribute("name","Default") != profileName)
    profileElem = profileElem.nextSiblingElement("profile"); 

  return profileElem;
}

QDomElement SettingsPlugin::setProfile(const QString &AProfile)
{
  if (isSettingsValid())
  {
    setProfileClosed();

    if (profiles().contains(AProfile)) 
      FProfile = profileNode(AProfile);
    else
      FProfile = profileNode();

    if (FProfile.isNull())
      FProfile = addProfile(FSettings.documentElement().attribute("profile","Default"));

    if (!FProfile.isNull())
    {
      FSettings.documentElement().setAttribute("profile",FProfile.attribute("name","Default"));
      setProfileOpened();
    }
  }
  return FProfile;
}

void SettingsPlugin::removeProfile(const QString &AProfile)
{
  if (AProfile == profile())
    setProfileClosed();

  QDomElement profileElem = profileNode(AProfile);
  if (!profileElem.isNull())
  {
    FSettings.documentElement().removeChild(profileElem);
    emit profileRemoved(AProfile);
  }
}

QDomElement SettingsPlugin::pluginNode(const QUuid &AId) 
{
  if (!FProfileOpened)
    return QDomElement();

  QDomElement node = FProfile.firstChildElement("plugin");  
  while (!node.isNull() && node.attribute("uid") != AId)
    node = node.nextSiblingElement("plugin");

  if (node.isNull())
  {
    node = FProfile.appendChild(FSettings.createElement("plugin")).toElement();
    node.setAttribute("uid",AId.toString()); 
  }

  return node;
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
    connect(FOptionsDialog, SIGNAL(accepted()),SLOT(onOptionsDialogAccepted()));
    connect(FOptionsDialog, SIGNAL(rejected()),SLOT(onOptionsDialogRejected()));

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

void SettingsPlugin::openOptionsDialogAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString node = action->data(Action::DR_Parametr1).toString();
    openOptionsDialog(node);
  }
}

QWidget *SettingsPlugin::createNodeWidget(const QString &ANode)
{
  QWidget *nodeWidget = new QWidget;
  QVBoxLayout *nodeLayout = new QVBoxLayout;
  nodeLayout->setMargin(6);
  nodeLayout->setSpacing(3);
  nodeWidget->setLayout(nodeLayout);
  
  QMap<int, QWidget *> widgetsByOrder;
  IOptionsHolder *optionsHolder;
  foreach(optionsHolder,FOptionsHolders)
  {
    int order = 500;
    QWidget *itemWidget = optionsHolder->optionsWidget(ANode,order);
    if (itemWidget)
      widgetsByOrder.insert(order,itemWidget);
  }
  
  QWidget *itemWidget;
  foreach(itemWidget,widgetsByOrder)
    nodeLayout->addWidget(itemWidget);
  nodeLayout->addStretch();
  
  return nodeWidget;
}

void SettingsPlugin::setProfileOpened()
{
  if (!FProfileOpened)
  {
    actOpenOptionsDialog->setEnabled(true);
    FProfileOpened = true;
    emit profileOpened(profile());
  }
}

void SettingsPlugin::setProfileClosed()
{
  if (FProfileOpened)
  {
    emit profileClosed(profile());
    FProfileOpened = false;
    FProfile.clear();
    actOpenOptionsDialog->setEnabled(false);
  }
}

void SettingsPlugin::onMainWindowCreated(IMainWindow *AMainWindow)
{
  AMainWindow->mainMenu()->addAction(actOpenOptionsDialog,SETTINGS_ACTION_GROUP_OPTIONS,true);
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

void SettingsPlugin::onPluginManagerQuit()
{
  if (!FOptionsDialog.isNull())
    FOptionsDialog->reject();

  if (FProfileOpened)
  {
    emit profileClosed(profile());
    saveSettings();
    FProfileOpened = false;
  }
}

Q_EXPORT_PLUGIN2(SettingsPlugin, SettingsPlugin)
