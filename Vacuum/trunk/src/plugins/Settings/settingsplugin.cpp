#include "settingsplugin.h"
#include <QtDebug>
#include <QTextStream>
#include <QByteArray>

SettingsPlugin::SettingsPlugin()
{
  FProfileOpened = false;
  FFile.setParent(this);
}

SettingsPlugin::~SettingsPlugin()
{
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
  connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onPluginManagerQuit()));

  return setFileName("config.xml");
}

bool SettingsPlugin::startPlugin()
{
  setProfile(QString::null);
  return true;
}

//ISettings
ISettings *SettingsPlugin::newSettings(const QUuid &APluginId, QObject *AParent)
{
  Settings *settings = new Settings(APluginId,this,AParent);
  FCleanupHandler.add(settings); 
  return settings;
}

bool SettingsPlugin::setFileName(const QString &AFileName)
{
  if (FFile.isOpen())
    FFile.close(); 

  if (FProfileOpened)
  {
    FProfileOpened = false;
    emit profileClosed();
  }

  FFile.setFileName(AFileName);

  if (!FFile.exists())
  {
    if (FFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      FFile.close(); 
      return true;
    }
  }
  else if (FFile.open(QIODevice::ReadOnly))
  {
    if (FFile.size() == 0 || FSettings.setContent(FFile.readAll(),true))
    {
      FFile.close();
      return true;
    }
    FFile.close();
  }
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

  QDomElement elem = FProfile.firstChildElement("plugin");  
  while (!elem.isNull() && elem.attribute("uid") != AId)
    elem = elem.nextSiblingElement("plugin");

  if (elem.isNull())
  {
    elem = FProfile.appendChild(FSettings.createElement("plugin")).toElement();
    elem.setAttribute("uid",AId.toString()); 
  }

  return elem;
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
