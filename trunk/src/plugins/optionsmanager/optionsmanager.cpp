#include "optionsmanager.h"

#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QCryptographicHash>
#include <definitions/actiongroups.h>
#include <definitions/commandline.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/version.h>
#include <utils/widgetmanager.h>
#include <utils/filestorage.h>
#include <utils/action.h>
#include <utils/logger.h>

#define DIR_PROFILES                    "profiles"
#define DIR_BINARY                      "binary"
#define FILE_PROFILE                    "profile.xml"
#define FILE_OPTIONS                    "options.xml"
#define FILE_OPTIONS_COPY               "options.xml.copy"
#define FILE_OPTIONS_FAIL               "options.xml.fail"
#define FILE_STATIC_OPTIONS             "static-options.xml"
#define FILE_INITIAL_OPTIONS            "initial-options.xml"
#define FILE_DEFAULT_OPTIONS            "default-options.xml"
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
}

bool OptionsManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onApplicationAboutToQuit()));

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
	FChangeProfileAction->setText(tr("Change Profile"));
	FChangeProfileAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_PROFILES);
	connect(FChangeProfileAction,SIGNAL(triggered(bool)),SLOT(onChangeProfileByAction(bool)));

	FShowOptionsDialogAction = new Action(this);
	FShowOptionsDialogAction->setText(tr("Options"));
	FShowOptionsDialogAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_DIALOG);
	FShowOptionsDialogAction->setEnabled(false);
	connect(FShowOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(onShowOptionsDialogByAction(bool)));

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FChangeProfileAction,AG_MMENU_OPTIONS,true);
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FShowOptionsDialogAction,AG_MMENU_OPTIONS,true);
	}

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FChangeProfileAction,AG_TMTM_OPTIONS,true);
		FTrayManager->contextMenu()->addAction(FShowOptionsDialogAction,AG_TMTM_OPTIONS,true);
	}

	return true;
}

bool OptionsManager::initSettings()
{
	Options::setDefaultValue(OPV_COMMON_AUTOSTART,false);
	Options::setDefaultValue(OPV_COMMON_LANGUAGE,QString());

	if (profiles().count() == 0)
		addProfile(DEFAULT_PROFILE, QString::null);

	IOptionsDialogNode commonNode = { ONO_COMMON, OPN_COMMON, MNI_OPTIONS_DIALOG, tr("Common") };
	insertOptionsDialogNode(commonNode);

	IOptionsDialogNode appearanceNode = { ONO_APPEARANCE, OPN_APPEARANCE, MNI_OPTIONS_APPEARANCE, tr("Appearance") };
	insertOptionsDialogNode(appearanceNode);

	insertOptionsDialogHolder(this);

	return true;
}

bool OptionsManager::startPlugin()
{
	updateOptionDefaults(loadAllOptionValues(FILE_DEFAULT_OPTIONS));

	QStringList args = qApp->arguments();
	int profIndex = args.indexOf(CLO_PROFILE);
	int passIndex = args.indexOf(CLO_PROFILE_PASSWORD);
	QString profile = profIndex>0 ? args.value(profIndex+1) : lastActiveProfile();
	QString password = passIndex>0 ? args.value(passIndex+1) : QString::null;
	if (profile.isEmpty() || !setCurrentProfile(profile, password))
		showLoginDialog();

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> OptionsManager::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (ANodeId == OPN_COMMON)
	{
		widgets.insertMulti(OHO_COMMON_SETTINGS, newOptionsDialogHeader(tr("Common settings"),AParent));
#ifdef Q_WS_WIN
		widgets.insertMulti(OWO_COMMON_AUTOSTART, newOptionsDialogWidget(Options::node(OPV_COMMON_AUTOSTART), tr("Auto run application on system startup"), AParent));
#else
		Q_UNUSED(AParent);
#endif

		widgets.insertMulti(OHO_COMMON_LOCALIZATION, newOptionsDialogHeader(tr("Localization"),AParent));

		QDir localeDir(QApplication::applicationDirPath());
		localeDir.cd(TRANSLATIONS_DIR);

		QComboBox *langCombox = new QComboBox(AParent);
		langCombox->addItem(tr("<System Language>"),QString());
		foreach(QString name, localeDir.entryList(QDir::Dirs,QDir::LocaleAware))
		{
			QLocale locale(name);
			if (locale.language() != QLocale::C)
			{
				QString langName = locale.nativeLanguageName();
				QString countryName = locale.nativeCountryName();
				if (!langName.isEmpty() && !countryName.isEmpty())
				{
					langName[0] = langName[0].toUpper();
					countryName[0] = countryName[0].toUpper();
					langCombox->addItem(QString("%1 (%2)").arg(langName,countryName),locale.name());
				}
			}
		}
		widgets.insertMulti(OWO_COMMON_LANGUAGE,newOptionsDialogWidget(Options::node(OPV_COMMON_LANGUAGE),tr("Language:"),langCombox,AParent));
	}
	return widgets;
}

bool OptionsManager::isOpened() const
{
	return !FProfile.isEmpty();
}

QList<QString> OptionsManager::profiles() const
{
	QList<QString> profileList;
	foreach(const QString &dirName, FProfilesDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
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
	foreach(const QString &profile, profiles())
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
	LOG_INFO(QString("Changing current profile to=%1").arg(AProfile));
	if (AProfile.isEmpty())
	{
		closeProfile();
		return true;
	}
	else if (AProfile == currentProfile())
	{
		return true;
	}
	else if (checkProfilePassword(AProfile,APassword))
	{
		closeProfile();
		FProfileLocker = new QtLockedFile(QDir(profilePath(AProfile)).absoluteFilePath(FILE_BLOCKER));
		if (FProfileLocker->open(QFile::WriteOnly) && FProfileLocker->lock(QtLockedFile::WriteLock,false))
		{
			bool isEmptyProfile = false;

			QDir profileDir(profilePath(AProfile));
			if (!profileDir.exists(DIR_BINARY))
				profileDir.mkdir(DIR_BINARY);

			// Loading options from file
			QString xmlError;
			QFile optionsFile(profileDir.filePath(FILE_OPTIONS));
			if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(&optionsFile,true,&xmlError))
			{
				if (!xmlError.isEmpty())
					REPORT_ERROR(QString("Failed to load options from file content: %1").arg(xmlError));
				else if (optionsFile.exists())
					REPORT_ERROR(QString("Failed to load options from file: %1").arg(optionsFile.errorString()));

				xmlError.clear();
				optionsFile.close();

				// Trying to open valid copy of options
				optionsFile.setFileName(profileDir.filePath(FILE_OPTIONS_COPY));
				if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(&optionsFile,true,&xmlError))
				{
					if (!xmlError.isEmpty())
						REPORT_ERROR(QString("Failed to load options backup from file content: %1").arg(xmlError));
					else if (optionsFile.exists())
						REPORT_ERROR(QString("Failed to load options backup from file: %1").arg(optionsFile.errorString()));

					isEmptyProfile = true;
					FProfileOptions.clear();
					FProfileOptions.appendChild(FProfileOptions.createElement("options")).toElement();

					LOG_INFO(QString("Created new options for profile=%1").arg(AProfile));
				}
				else
				{
					LOG_INFO(QString("Options loaded from backup for profile=%1").arg(AProfile));
				}

				// Renaming invalid options file
				QFile::remove(profileDir.filePath(FILE_OPTIONS_FAIL));
				QFile::rename(profileDir.filePath(FILE_OPTIONS),profileDir.filePath(FILE_OPTIONS_FAIL));
			}
			else
			{
				// Saving the copy of valid options
				QFile::remove(profileDir.filePath(FILE_OPTIONS_COPY));
				QFile::copy(profileDir.filePath(FILE_OPTIONS),profileDir.filePath(FILE_OPTIONS_COPY));
			}
			optionsFile.close();

			if (profileKey(AProfile,APassword).size() < 16)
				changeProfilePassword(AProfile,APassword,APassword);

			if (isEmptyProfile)
				updateOptionValues(loadAllOptionValues(FILE_INITIAL_OPTIONS));
			updateOptionValues(loadAllOptionValues(FILE_STATIC_OPTIONS));

			openProfile(AProfile, APassword);
			return true;
		}
		FProfileLocker->close();
		delete FProfileLocker;
	}
	else
	{
		LOG_WARNING(QString("Failed to change current profile to=%1: Invalid password").arg(AProfile));
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

		if (saveProfile(AProfile, profileDoc))
			LOG_INFO(QString("Profile password changed, profile=%1").arg(AProfile));
		else
			LOG_ERROR(QString("Failed to change profile password, profile=%1: Profile not saved").arg(AProfile));
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
				LOG_INFO(QString("New profile added, profile=%1").arg(AProfile));
				emit profileAdded(AProfile);
				return true;
			}
			else
			{
				LOG_ERROR(QString("Failed to add new profile, profile=%1: Profile not saved").arg(AProfile));
			}
		}
		else
		{
			REPORT_ERROR("Failed to add new profile: Directory not created");
		}
	}
	return false;
}

bool OptionsManager::renameProfile(const QString &AProfile, const QString &ANewName)
{
	if (!FProfilesDir.exists(ANewName) && FProfilesDir.rename(AProfile, ANewName))
	{
		LOG_INFO(QString("Profile renamed from=%1 to=%2").arg(AProfile,ANewName));
		emit profileRenamed(AProfile,ANewName);
		return true;
	}
	else
	{
		LOG_ERROR(QString("Failed to rename profile=%1 to=%2: Directory not renamed").arg(AProfile,ANewName));
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
			LOG_INFO(QString("Profile removed, profile=%1").arg(AProfile));
			emit profileRemoved(AProfile);
			return true;
		}
		else
		{
			LOG_ERROR(QString("Failed to remove profile=%1: Directory not removed").arg(AProfile));
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
	WidgetManager::showActivateRaiseWindow(FLoginDialog);
	return FLoginDialog;
}

QDialog *OptionsManager::showEditProfilesDialog(QWidget *AParent)
{
	if (FEditProfilesDialog.isNull())
		FEditProfilesDialog = new EditProfilesDialog(this,AParent);
	WidgetManager::showActivateRaiseWindow(FEditProfilesDialog);
	return FEditProfilesDialog;
}

QList<IOptionsDialogHolder *> OptionsManager::optionsDialogHolders() const
{
	return FOptionsHolders;
}

void OptionsManager::insertOptionsDialogHolder(IOptionsDialogHolder *AHolder)
{
	if (!FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.append(AHolder);
		emit optionsDialogHolderInserted(AHolder);
	}
}

void OptionsManager::removeOptionsDialogHolder(IOptionsDialogHolder *AHolder)
{
	if (FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.removeAll(AHolder);
		emit optionsDialogHolderRemoved(AHolder);
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
		LOG_DEBUG(QString("Options node inserted, id=%1").arg(ANode.nodeId));
		FOptionsDialogNodes[ANode.nodeId] = ANode;
		emit optionsDialogNodeInserted(ANode);
	}
}

void OptionsManager::removeOptionsDialogNode(const QString &ANodeId)
{
	if (FOptionsDialogNodes.contains(ANodeId))
	{
		LOG_DEBUG(QString("Options node removed, id=%1").arg(ANodeId));
		emit optionsDialogNodeRemoved(FOptionsDialogNodes.take(ANodeId));
	}
}

QDialog *OptionsManager::showOptionsDialog(const QString &ANodeId, const QString &ARootId, QWidget *AParent)
{
	if (isOpened())
	{
		QPointer<OptionsDialog> &dialog = FOptionDialogs[ARootId];
		if (dialog.isNull())
		{
			dialog = new OptionsDialog(this,ARootId,AParent);
			connect(dialog,SIGNAL(applied()),SLOT(onOptionsDialogApplied()),Qt::QueuedConnection);
		}
		dialog->showNode(ANodeId.isNull() ? Options::fileValue("options.dialog.last-node",ARootId).toString() : ANodeId);
		WidgetManager::showActivateRaiseWindow(dialog);
		return dialog;
	}
	return NULL;
}

IOptionsDialogWidget *OptionsManager::newOptionsDialogHeader(const QString &ACaption, QWidget *AParent) const
{
	return new OptionsDialogHeader(ACaption,AParent);
}

IOptionsDialogWidget *OptionsManager::newOptionsDialogWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const
{
	return new OptionsDialogWidget(ANode, ACaption, AParent);
}

IOptionsDialogWidget *OptionsManager::newOptionsDialogWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AEditor, QWidget *AParent) const
{
	return new OptionsDialogWidget(ANode,ACaption,AEditor,AParent);
}

void OptionsManager::closeProfile()
{
	if (isOpened())
	{
		LOG_INFO(QString("Closing profile=%1").arg(FProfile));
		emit profileClosed(currentProfile());

		FAutoSaveTimer.stop();
		qDeleteAll(FOptionDialogs);
		FShowOptionsDialogAction->setEnabled(false);

		Options::setOptions(QDomDocument(), QString::null, QByteArray());
		saveCurrentProfileOptions();

		FProfile.clear();
		FProfileKey.clear();
		FProfileOptions.clear();
		FProfileLocker->unlock();
		FProfileLocker->close();
		FProfileLocker->remove();
		delete FProfileLocker;
	}
}

void OptionsManager::openProfile(const QString &AProfile, const QString &APassword)
{
	if (!isOpened())
	{
		LOG_INFO(QString("Opening profile=%1").arg(AProfile));

		FProfile = AProfile;
		FProfileKey = profileKey(AProfile, APassword);
		Options::setOptions(FProfileOptions, profilePath(AProfile) + "/" DIR_BINARY, FProfileKey);
		
		FAutoSaveTimer.start();
		FShowOptionsDialogAction->setEnabled(true);

		emit profileOpened(AProfile);
	}
}

QDomDocument OptionsManager::profileDocument(const QString &AProfile) const
{
	QDomDocument doc;
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::ReadOnly))
	{
		QString xmlError;
		if (!doc.setContent(&file,true,&xmlError))
		{
			REPORT_ERROR(QString("Failed to load profile options from file content: %1").arg(xmlError));
			doc.clear();
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load profile options from file: %1").arg(file.errorString()));
	}
	return doc;
}

bool OptionsManager::saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const
{
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::WriteOnly|QFile::Truncate))
	{
		LOG_INFO(QString("Profile options saved, profile=%1").arg(AProfile));
		file.write(AProfileDoc.toString(2).toUtf8());
		file.flush();
		return true;
	}
	else
	{
		REPORT_ERROR(QString("Failed to save profile options to file: %1").arg(file.errorString()));
	}
	return false;
}

bool OptionsManager::saveCurrentProfileOptions() const
{
	if (isOpened())
	{
		QFile file(QDir(profilePath(FProfile)).filePath(FILE_OPTIONS));
		if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
		{
			LOG_DEBUG(QString("Current profile options saved, profile=%1").arg(FProfile));
			file.write(FProfileOptions.toString(2).toUtf8());
			file.close();
			return true;
		}
		else
		{
			REPORT_ERROR(QString("Failed to save current profile options to file: %1").arg(file.errorString()));
		}
	}
	else
	{
		REPORT_ERROR("Failed to save current profile options: Profile not opened");
	}
	return false;
}

QMap<QString, QVariant> OptionsManager::getOptionValues(const OptionsNode &ANode) const
{
	QMap<QString,QVariant> values;

	if (ANode.hasValue())
		values.insert(ANode.path(),ANode.value());

	foreach(const QString &childName, ANode.childNames())
		foreach(const QString &childNs, ANode.childNSpaces(childName))
			values.unite(getOptionValues(ANode.node(childName,childNs)));

	return values;
}

QMap<QString, QVariant> OptionsManager::loadOptionValues(const QString &AFilePath) const
{
	QFile file(AFilePath);
	if (file.open(QFile::ReadOnly))
	{
		QByteArray data = file.readAll();

		foreach(const QString &env, QProcess::systemEnvironment())
		{
			int keyPos = env.indexOf('=');
			if (keyPos > 0)
			{
				QString envKey = "%"+env.left(keyPos)+"%";
				QString envVal = env.right(env.length() - keyPos - 1);
				data.replace(envKey.toUtf8(),envVal.toUtf8());
			}
		}

		QString xmlError;
		QDomDocument doc;
		if (doc.setContent(data,true,&xmlError))
		{
			if (doc.documentElement().tagName() == "options")
			{
				LOG_INFO(QString("Option values loaded from file=%1").arg(AFilePath));
				return getOptionValues(Options::createNodeForElement(doc.documentElement()));
			}
			else
			{
				LOG_ERROR(QString("Failed to load option values from file=%1 content: Invalid tagname").arg(file.fileName()));
			}
		}
		else
		{
			LOG_ERROR(QString("Failed to load option values from file=%1 content: %2").arg(file.fileName(),xmlError));
		}
	}
	else if (file.exists())
	{
		LOG_ERROR(QString("Failed to load option values from file=%1: %2").arg(file.fileName(),file.errorString()));
	}
	return QMap<QString,QVariant>();
}

QMap<QString, QVariant> OptionsManager::loadAllOptionValues(const QString &AFileName) const
{
	QMap<QString, QVariant> options;
	foreach(const QString &resDir, FileStorage::resourcesDirs())
	{
		QDir dir(resDir);
		options.unite(loadOptionValues(dir.absoluteFilePath(AFileName)));
	}
	return options;
}

void OptionsManager::updateOptionValues(const QMap<QString, QVariant> &AOptions) const
{
	Options::instance()->blockSignals(true);

	OptionsNode rootNode = Options::createNodeForElement(FProfileOptions.documentElement());
	for (QMap<QString,QVariant>::const_iterator it=AOptions.constBegin(); it!=AOptions.constEnd(); ++it)
		rootNode.setValue(it.value(),it.key());

	Options::instance()->blockSignals(false);
}

void OptionsManager::updateOptionDefaults(const QMap<QString, QVariant> &AOptions) const
{
	for (QMap<QString,QVariant>::const_iterator it=AOptions.constBegin(); it!=AOptions.constEnd(); ++it)
		Options::setDefaultValue(it.key(),it.value());
}

void OptionsManager::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_COMMON_AUTOSTART)
	{
#ifdef Q_WS_WIN
		QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
		if (ANode.value().toBool())
			reg.setValue(CLIENT_NAME, QApplication::arguments().join(" "));
		else
			reg.remove(CLIENT_NAME);
#endif
	}
	else if (ANode.path() == OPV_COMMON_LANGUAGE)
	{
		QLocale locale(ANode.value().toString());
		FPluginManager->setLocale(locale.language(),locale.country());
	}

	LOG_DEBUG(QString("Options node value changed, node=%1, value=%2").arg(ANode.path(),ANode.value().toString()));
}

void OptionsManager::onOptionsDialogApplied()
{
	saveCurrentProfileOptions();
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
	saveCurrentProfileOptions();
}

void OptionsManager::onApplicationAboutToQuit()
{
	closeProfile();
}

Q_EXPORT_PLUGIN2(plg_optionsmanager, OptionsManager)
