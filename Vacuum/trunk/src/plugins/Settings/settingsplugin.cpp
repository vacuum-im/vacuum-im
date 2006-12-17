#include "settingsplugin.h"
#include <QtDebug>
#include <QTextStream>
#include <QByteArray>

SettingsPlugin::SettingsPlugin()
{
  FProfileOpened = false;
}

SettingsPlugin::~SettingsPlugin()
{
  qDebug() << "~SettingsPlugin";
  FCleanupHandler.clear(); 
}

void SettingsPlugin::getPluginInfo(PluginInfo *info)
{
  info->author = "Potapov S.A. aka Lion";
  info->description = tr("Managing profiles and settings");
  info->homePage = "http://jrudevels.org";
  info->name = tr("Settings Manager"); 
  info->uid = SETTINGS_UUID;
  info ->version = "0.1";
}

bool SettingsPlugin::initPlugin(IPluginManager *APluginManager)
{
  FPluginManager = APluginManager;

  QObject *obj = FPluginManager->instance();
  connect(obj,SIGNAL(aboutToQuit()),SLOT(onPluginManagerQuit()));

  FFile.setParent(this);
  bool ready = setFileName("config.xml");
  if (!ready)
    qDebug() << "CANT OPEN CONFIG FILE " << FFile.fileName();
  return ready;
}

bool SettingsPlugin::startPlugin()
{
  setProfile(QString::null);
  return true;
}

//ISettings
ISettings *SettingsPlugin::newSettings(const QUuid &AUuid, QObject *parent)
{
  Settings *settings = new Settings(AUuid,this,parent);
  FCleanupHandler.add(settings); 
  return settings;
}

bool SettingsPlugin::setFileName(const QString &AFileName)
{
  if (FFile.isOpen())
    FFile.close(); 

  FFile.setFileName(AFileName);
  if (!FFile.exists())
  {
    FFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    FFile.close(); 
    return true;
  }

  if (FFile.open(QIODevice::ReadOnly))
  {
    if (FProfileOpened)
    {
      FProfileOpened = false;
      emit profileClosed();
    }

    if (FFile.size() == 0 || FSettings.setContent(FFile.readAll(),true))
    {
      FFile.close();
      return true;
    }

    qDebug() << "WRONG SETTINGS FORMAT IN:" << AFileName;
    FFile.close();
    return false;
  }
  qDebug() << "CANT OPEN SETTINGS IN FILE" << FFile.fileName(); 
  return false;
}

bool SettingsPlugin::saveSettings()
{
  if (FFile.isOpen())
    FFile.close(); 

  if (FFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    FFile.write(FSettings.toByteArray());  
    FFile.close(); 
    return true;
  }
  qDebug() << "CANT SAVE SETTINGS TO FILE" << FFile.fileName(); 
  return false;
}

QDomElement SettingsPlugin::setProfile(const QString &AProfile)
{
  if (FProfileOpened)
  {
    FProfileOpened = false;
    emit profileClosed();
  }

  if (!AProfile.isEmpty()) 
    FProfile = getProfile(AProfile);
  else
    FProfile = getProfile(FSettings.documentElement().attribute("profile","Default"));

  if (!FProfile.isNull())
  {
    FProfileOpened = true;
    FSettings.documentElement().setAttribute("profile",FProfile.attribute("name","Default"));
    emit profileOpened();
  }
  return FProfile;
}

QDomElement SettingsPlugin::getProfile(const QString &AProfile) 
{
  if (AProfile.isEmpty())
    return QDomElement();

  if (FSettings.isNull())
    FSettings.appendChild(FSettings.createElement("config")).toElement().setAttribute("profile","Default");  
  
  QDomNode profile = FSettings.documentElement().firstChild();  
  while (!profile.isNull())
  {
    if (profile.isElement())
      if (profile.toElement().tagName() == "profile")
        if (profile.toElement().attribute("name","Default") == AProfile)
          return profile.toElement(); 
    profile = profile.nextSibling(); 
  }

  profile = FSettings.documentElement().appendChild(FSettings.createElement("profile"));
  profile.toElement().setAttribute("name",AProfile); 
  return profile.toElement();
}

QDomElement SettingsPlugin::getPluginNode(const QUuid &AId) 
{
  if (FProfile.isNull())
    return QDomElement();

  QDomNode node = FProfile.firstChild();  
  while (!node.isNull())
  {
    if (node.isElement())
      if (node.toElement().tagName() == "plugin")
        if (node.toElement().attribute("uid") == AId)
          return node.toElement(); 
    node = node.nextSibling(); 
  }
  node = FProfile.appendChild(FSettings.createElement("plugin"));
  node.toElement().setAttribute("uid",AId.toString()); 
  return node.toElement();
}

void SettingsPlugin::onPluginManagerQuit()
{
  if (FProfileOpened)
  {
    emit profileClosed();
    saveSettings();
    FProfileOpened = false;
  }
}

Q_EXPORT_PLUGIN2(SettingsPlugin, SettingsPlugin)
