#include "optionsmanager.h"

#include <QtDebug>

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QCryptographicHash>

#define DIR_PROFILES                    "profiles"
#define DIR_BINARY                      "binary"
#define FILE_PROFILE                    "profile.xml"
#define FILE_OPTIONS                    "options.xml"
#define FILE_BLOCKER                    "blocked"

#define PROFILE_VERSION                 "1.0"
#define DEFAULT_PROFILE                 "Default"

#define ADR_PROFILE                     Action::DR_Parametr1

OptionsManager::OptionsManager()
{
  FPluginManager = NULL;
  FTrayManager = NULL;
  FMainWindowPlugin = NULL;

  FAutoSaveTimer.setInterval(5*60*1000);
  FAutoSaveTimer.setSingleShot(false);
  connect(&FAutoSaveTimer, SIGNAL(timeout()),SLOT(onAutoSaveTimerTimeout()));

  qsrand(QDateTime::currentDateTime().toTime_t());
}

OptionsManager::~OptionsManager()
{

}

void OptionsManager::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Options Manager"); 
  APluginInfo->description = tr("Allows to save, load and manage user preferences");
  APluginInfo ->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
  APluginInfo->conflicts.append("{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"); // ISettings
}

bool OptionsManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  Q_UNUSED(AInitOrder);

  FPluginManager = APluginManager;
  connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onAboutToQuit()));

  IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

  return true;
}

bool OptionsManager::initObjects()
{
  FProfilesDir.setPath(FPluginManager->homePath());
  if (!FProfilesDir.exists(DIR_PROFILES))
    FProfilesDir.mkdir(DIR_PROFILES);
  FProfilesDir.cd(DIR_PROFILES);

  FChangeProfileAction = new Action(this);
  FChangeProfileAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_PROFILES);
  FChangeProfileAction->setText(tr("Change Profile"));
  connect(FChangeProfileAction,SIGNAL(triggered(bool)),SLOT(onChangeProfileByAction(bool)));

  FShowOptionsDialogAction = new Action(this);
  FShowOptionsDialogAction->setEnabled(false);
  FShowOptionsDialogAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_DIALOG);
  FShowOptionsDialogAction->setText(tr("Options..."));
  connect(FShowOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(onShowOptionsDialogByAction(bool)));

  if (FMainWindowPlugin)
  {
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FChangeProfileAction,AG_MMENU_OPTIONS,true);
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FShowOptionsDialogAction,AG_MMENU_OPTIONS,true);
  }

  if (FTrayManager)
  {
    FTrayManager->addAction(FChangeProfileAction,AG_TMTM_OPTIONS,true);
    FTrayManager->addAction(FShowOptionsDialogAction,AG_TMTM_OPTIONS,true);
  }

  return true;
}

bool OptionsManager::initSettings()
{
  Options::setDefaultValue(OPV_MISC_AUTOSTART, false);

  if (profiles().count() == 0)
    addProfile(DEFAULT_PROFILE, QString::null);

  IOptionsDialogNode miscNode = { ONO_MISC, OPN_MISC, tr("Misc"), tr("Extra options"), MNI_OPTIONS_DIALOG };
  insertOptionsDialogNode(miscNode);
  insertOptionsHolder(this);

  return true;
}

bool OptionsManager::startPlugin()
{
  QStringList args = qApp->arguments();
  int profIndex = args.indexOf(CLO_PROFILE);
  int passIndex = args.indexOf(CLO_PROFILE_PASSWORD);
  QString profile = profIndex>0 ? args.value(profIndex+1) : lastActiveProfile();
  QString password = passIndex>0 ? args.value(passIndex+1) : QString::null;
  if (profile.isEmpty() || !setCurrentProfile(profile, password))
    showLoginDialog();
  return true;
}

IOptionsWidget *OptionsManager::optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent)
{
  if (ANodeId == OPN_MISC)
  {
    AOrder = OWO_MISC_AUTOSTART;
    return optionsNodeWidget(Options::node(OPV_MISC_AUTOSTART), tr("Auto run on system startup"), AParent);
  }
  return NULL;
}

bool OptionsManager::isOpened() const
{
  return !FProfile.isEmpty();
}

QList<QString> OptionsManager::profiles() const
{
  QList<QString> profileList;
  
  foreach(QString dirName, FProfilesDir.entryList(QDir::Dirs))
    if (FProfilesDir.exists(dirName + "/" FILE_PROFILE))
      profileList.append(dirName);

  return profileList;
}

QString OptionsManager::profilePath(const QString &AProfile) const
{
  return FProfilesDir.absoluteFilePath(AProfile);
}

QString OptionsManager::lastActiveProfile() const
{
  QDateTime lastModified;
  QString lastProfile = DEFAULT_PROFILE;
  foreach(QString profile, profiles())
  {
    QFileInfo info(profilePath(profile) + "/" FILE_OPTIONS);
    if (info.exists() && info.lastModified()>lastModified)
    {
      lastProfile = profile;
      lastModified = info.lastModified();
    }
  }
  return lastProfile;
}

QString OptionsManager::currentProfile() const
{
  return FProfile;
}

QByteArray OptionsManager::currentProfileKey() const
{
  return FProfileKey;
}

bool OptionsManager::setCurrentProfile(const QString &AProfile, const QString &APassword)
{
  if (AProfile.isEmpty())
  {
    closeProfile();
    return true;
  }
  else if (AProfile == currentProfile())
  {
    return true;
  }
  else if (checkProfilePassword(AProfile, APassword))
  {
    closeProfile();
    FProfileLocker.setFileName(QDir(profilePath(AProfile)).absoluteFilePath(FILE_BLOCKER));
    if (FProfileLocker.open(QFile::WriteOnly) && FProfileLocker.lock(QtLockedFile::WriteLock, false))
    {
      QDir profileDir(profilePath(AProfile));
      if (!profileDir.exists(DIR_BINARY))
        profileDir.mkdir(DIR_BINARY);

      QFile optionsFile(profileDir.filePath(FILE_OPTIONS));
      if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(optionsFile.readAll(),true))
      {
        FProfileOptions.clear();
        FProfileOptions.appendChild(FProfileOptions.createElement("options")).toElement();
      }
      optionsFile.close();

      if (profileKey(AProfile,APassword).size() < 16)
        changeProfilePassword(AProfile,APassword,APassword);

      openProfile(AProfile, APassword);
      return true;
    }
    FProfileLocker.close();
  }
  return false;
}

QByteArray OptionsManager::profileKey(const QString &AProfile, const QString &APassword) const
{
  if (checkProfilePassword(AProfile, APassword))
  {
    QDomNode keyText = profileDocument(AProfile).documentElement().firstChildElement("key").firstChild();
    while (!keyText.isNull() && !keyText.isText())
      keyText = keyText.nextSibling();

    QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
    return Options::decrypt(keyValue, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
  }
  return QByteArray();
}

bool OptionsManager::checkProfilePassword(const QString &AProfile, const QString &APassword) const
{
  QDomDocument profileDoc = profileDocument(AProfile);
  if (!profileDoc.isNull())
  {
    QDomNode passText = profileDoc.documentElement().firstChildElement("password").firstChild();
    while (!passText.isNull() && !passText.isText())
      passText = passText.nextSibling();

    if (passText.isNull() && APassword.isEmpty())
      return true;

    QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
    return passHash.toHex() == passText.toText().data().toLatin1();
  }
  return false;
}

bool OptionsManager::changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword)
{
  if (checkProfilePassword(AProfile, AOldPassword))
  {
    QDomDocument profileDoc = profileDocument(AProfile);

    QDomElement passElem = profileDoc.documentElement().firstChildElement("password");
    if (passElem.isNull())
      passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password")).toElement();
    
    QDomNode passText = passElem.firstChild();
    while (!passText.isNull() && !passText.isText())
      passText = passText.nextSibling();
    
    QByteArray newPassHash = QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Sha1);
    if (passText.isNull())
      passElem.appendChild(passElem.ownerDocument().createTextNode(newPassHash.toHex()));
    else
      passText.toText().setData(newPassHash.toHex());

    QDomNode keyText = profileDoc.documentElement().firstChildElement("key").firstChild();
    while (!keyText.isNull() && !keyText.isText())
      keyText = keyText.nextSibling();
    
    QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
    keyValue = Options::decrypt(keyValue, QCryptographicHash::hash(AOldPassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
    if (keyValue.size() < 16)
    {
      keyValue.resize(16);
      for (int i=0; i<keyValue.size(); i++)
        keyValue[i] = qrand();
    }
    keyValue = Options::encrypt(keyValue, QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Md5));
    keyText.toText().setData(keyValue.toBase64());

    return saveProfile(AProfile, profileDoc);
  }
  return false;
}

bool OptionsManager::addProfile(const QString &AProfile, const QString &APassword)
{
  if (!profiles().contains(AProfile))
  {
    if (FProfilesDir.exists(AProfile) || FProfilesDir.mkdir(AProfile))
    {
      QDomDocument profileDoc;
      profileDoc.appendChild(profileDoc.createElement("profile"));
      profileDoc.documentElement().setAttribute("version",PROFILE_VERSION);

      QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
      QDomNode passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password"));
      passElem.appendChild(profileDoc.createTextNode(passHash.toHex()));

      QByteArray keyData(16,0);
      for (int i=0; i<keyData.size(); i++)
        keyData[i] = qrand();
      keyData = Options::encrypt(keyData, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5));

      QDomNode keyElem = profileDoc.documentElement().appendChild(profileDoc.createElement("key"));
      keyElem.appendChild(profileDoc.createTextNode(keyData.toBase64()));

      if (saveProfile(AProfile, profileDoc))
      {
        emit profileAdded(AProfile);
        return true;
      }
    }
  }
  return false;
}

bool OptionsManager::renameProfile(const QString &AProfile, const QString &ANewName)
{
  if (!FProfilesDir.exists(ANewName) && FProfilesDir.rename(AProfile, ANewName))
  {
    emit profileRenamed(AProfile,ANewName);
    return true;
  }
  return false;
}

bool OptionsManager::removeProfile(const QString &AProfile)
{
  QDir profileDir(profilePath(AProfile));
  if (profileDir.exists())
  {
    if (AProfile == currentProfile())
      closeProfile();
    
    if (profileDir.remove(FILE_PROFILE))
    {
      emit profileRemoved(AProfile);
      return true;
    }
  }
  return false;
}

QDialog *OptionsManager::showLoginDialog(QWidget *AParent)
{
  if (FLoginDialog.isNull())
  {
    FLoginDialog = new LoginDialog(this,AParent);
    connect(FLoginDialog,SIGNAL(rejected()),SLOT(onLoginDialogRejected()));
  }
  FLoginDialog->show();
  WidgetManager::raiseWidget(FLoginDialog);
  FLoginDialog->activateWindow();
  return FLoginDialog;
}

QDialog *OptionsManager::showEditProfilesDialog(QWidget *AParent)
{
  if (FEditProfilesDialog.isNull())
    FEditProfilesDialog = new EditProfilesDialog(this,AParent);
  FEditProfilesDialog->show();
  WidgetManager::raiseWidget(FEditProfilesDialog);
  FEditProfilesDialog->activateWindow();
  return FEditProfilesDialog;
}

QList<IOptionsHolder *> OptionsManager::optionsHolders() const
{
  return FOptionsHolders;
}

void OptionsManager::insertOptionsHolder(IOptionsHolder *AHolder)
{
  if (!FOptionsHolders.contains(AHolder))
  {
    FOptionsHolders.append(AHolder);
    emit optionsHolderInserted(AHolder);
  }
}

void OptionsManager::removeOptionsHolder(IOptionsHolder *AHolder)
{
  if (FOptionsHolders.contains(AHolder))
  {
    FOptionsHolders.removeAll(AHolder);
    emit optionsHolderRemoved(AHolder);
  }
}

QList<IOptionsDialogNode> OptionsManager::optionsDialogNodes() const
{
  return FOptionsDialogNodes.values();
}

IOptionsDialogNode OptionsManager::optionsDialogNode(const QString &ANodeId) const
{
  return FOptionsDialogNodes.value(ANodeId);
}

void OptionsManager::insertOptionsDialogNode(const IOptionsDialogNode &ANode)
{
  if (!ANode.nodeId.isEmpty())
  {
    FOptionsDialogNodes[ANode.nodeId] = ANode;
    emit optionsDialogNodeInserted(ANode);
  }
}

void OptionsManager::removeOptionsDialogNode(const QString &ANodeId)
{
  if (FOptionsDialogNodes.contains(ANodeId))
  {
    emit optionsDialogNodeRemoved(FOptionsDialogNodes.take(ANodeId));
  }
}

QDialog *OptionsManager::showOptionsDialog(const QString &ANodeId, QWidget *AParent)
{
  if (isOpened())
  {
    if (FOptionsDialog.isNull())
      FOptionsDialog = new OptionsDialog(this,AParent);
    FOptionsDialog->showNode(ANodeId);
    FOptionsDialog->show();
    WidgetManager::raiseWidget(FOptionsDialog);
    FOptionsDialog->activateWindow();
  }
  return FOptionsDialog;
}

IOptionsContainer *OptionsManager::optionsContainer(QWidget *AParent) const
{
  return new OptionsContainer(this,AParent);
}

IOptionsWidget *OptionsManager::optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const
{
  return new OptionsWidget(ANode, ACaption, AParent);
}

void OptionsManager::openProfile(const QString &AProfile, const QString &APassword)
{
  if (!isOpened())
  {
    FProfile = AProfile;
    FProfileKey = profileKey(AProfile, APassword);
    Options::setOptions(FProfileOptions, profilePath(AProfile) + "/" DIR_BINARY, FProfileKey);
    FAutoSaveTimer.start();
    FShowOptionsDialogAction->setEnabled(true);
    emit profileOpened(AProfile);
  }
}

void OptionsManager::closeProfile()
{
  if (isOpened())
  {
    emit profileClosed(currentProfile());
    FAutoSaveTimer.stop();
    if (!FOptionsDialog.isNull())
      FOptionsDialog->reject();
    FShowOptionsDialogAction->setEnabled(false);
    Options::setOptions(QDomDocument(), QString::null, QByteArray());
    saveOptions();
    FProfile.clear();
    FProfileKey.clear();
    FProfileOptions.clear();
    FProfileLocker.unlock();
    FProfileLocker.close();
    FProfileLocker.remove();
  }
}

bool OptionsManager::saveOptions() const
{
  if (isOpened())
  {
    QFile file(QDir(profilePath(currentProfile())).filePath(FILE_OPTIONS));
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
      file.write(FProfileOptions.toString(2).toUtf8());
      file.close();
      return true;
    }
  }
  return false;
}

bool OptionsManager::saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const
{
  QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
  if (file.open(QFile::WriteOnly|QFile::Truncate))
  {
    file.write(AProfileDoc.toString(2).toUtf8());
    file.close();
    return true;
  }
  return false;
}

QDomDocument OptionsManager::profileDocument(const QString &AProfile) const
{
  QDomDocument doc;
  QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
  if (file.open(QFile::ReadOnly))
  {
    doc.setContent(file.readAll(),true);
    file.close();
  }
  return doc;
}

void OptionsManager::onOptionsChanged(const OptionsNode &ANode)
{
  if (ANode.path() == OPV_MISC_AUTOSTART)
  {
#ifdef Q_WS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (ANode.value().toBool())
      reg.setValue(CLIENT_NAME, QDir::toNativeSeparators(QApplication::applicationFilePath()));
    else
      reg.remove(CLIENT_NAME);
#endif
  }
}

void OptionsManager::onChangeProfileByAction(bool)
{
  showLoginDialog();
}

void OptionsManager::onShowOptionsDialogByAction(bool)
{
  showOptionsDialog();
}

void OptionsManager::onLoginDialogRejected()
{
  if (!isOpened())
    FPluginManager->quit();
}

void OptionsManager::onAutoSaveTimerTimeout()
{
  saveOptions();
}

void OptionsManager::onAboutToQuit()
{
  closeProfile();
}

Q_EXPORT_PLUGIN2(plg_optionsmanager, OptionsManager)
