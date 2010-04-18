#ifndef OPTIONSMANAGER_H
#define OPTIONSMANAGER_H

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QPointer>
#include <definations/actiongroups.h>
#include <definations/commandline.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/version.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <utils/action.h>
#include <utils/widgetmanager.h>
#include <thirdparty/qtlockedfile/qtlockedfile.h>
#include "logindialog.h"
#include "editprofilesdialog.h"
#include "optionswidget.h"
#include "optionscontainer.h"
#include "optionsdialog.h"

class OptionsManager : 
  public QObject,
  public IPlugin,
  public IOptionsManager,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IOptionsManager IOptionsHolder);
public:
  OptionsManager();
  ~OptionsManager();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return OPTIONSMANAGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin();
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IOptionsManager
  virtual bool isOpened() const;
  virtual QList<QString> profiles() const;
  virtual QString profilePath(const QString &AProfile) const;
  virtual QString lastActiveProfile() const;
  virtual QString currentProfile() const;
  virtual QByteArray currentProfileKey() const;
  virtual bool setCurrentProfile(const QString &AProfile, const QString &APassword);
  virtual QByteArray profileKey(const QString &AProfile, const QString &APassword) const;
  virtual bool checkProfilePassword(const QString &AProfile, const QString &APassword) const;
  virtual bool changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword);
  virtual bool addProfile(const QString &AProfile, const QString &APassword);
  virtual bool renameProfile(const QString &AProfile, const QString &ANewName);
  virtual bool removeProfile(const QString &AProfile);
  virtual QDialog *showLoginDialog(QWidget *AParent = NULL);
  virtual QDialog *showEditProfilesDialog(QWidget *AParent = NULL);
  virtual QList<IOptionsHolder *> optionsHolders() const;
  virtual void insertOptionsHolder(IOptionsHolder *AHolder);
  virtual void removeOptionsHolder(IOptionsHolder *AHolder);
  virtual QList<IOptionsDialogNode> optionsDialogNodes() const;
  virtual IOptionsDialogNode optionsDialogNode(const QString &ANodeId) const;
  virtual void insertOptionsDialogNode(const IOptionsDialogNode &ANode);
  virtual void removeOptionsDialogNode(const QString &ANodeId);
  virtual QDialog *showOptionsDialog(const QString &ANodeId = QString::null, QWidget *AParent = NULL);
  virtual IOptionsContainer *optionsContainer(QWidget *AParent) const;
  virtual IOptionsWidget *optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const;
signals:
  void profileAdded(const QString &AProfile);
  void profileOpened(const QString &AProfile);
  void profileClosed(const QString &AProfile);
  void profileRenamed(const QString &AProfile, const QString &ANewName);
  void profileRemoved(const QString &AProfile);
  void optionsHolderInserted(IOptionsHolder *AHolder);
  void optionsHolderRemoved(IOptionsHolder *AHolder);
  void optionsDialogNodeInserted(const IOptionsDialogNode &ANode);
  void optionsDialogNodeRemoved(const IOptionsDialogNode &ANode);
protected:
  void openProfile(const QString &AProfile, const QString &APassword);
  void closeProfile();
  bool saveOptions() const;
  bool saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const;
  QDomDocument profileDocument(const QString &AProfile) const;
protected slots:
  void onOptionsChanged(const OptionsNode &ANode);
  void onChangeProfileByAction(bool);
  void onShowOptionsDialogByAction(bool);
  void onLoginDialogRejected();
  void onAutoSaveTimerTimeout();
  void onAboutToQuit();
private:
  IPluginManager *FPluginManager;
  ITrayManager *FTrayManager;
  IMainWindowPlugin *FMainWindowPlugin;
private:
  QDir FProfilesDir;
  QTimer FAutoSaveTimer;
private:
  QString FProfile;
  QByteArray FProfileKey;
  QDomDocument FProfileOptions;
  QtLockedFile FProfileLocker;
private:
  Action *FChangeProfileAction;
  QPointer<LoginDialog> FLoginDialog;
  QPointer<EditProfilesDialog> FEditProfilesDialog;
private:
  Action *FShowOptionsDialogAction;
  QList<IOptionsHolder *> FOptionsHolders;
  QMap<QString, IOptionsDialogNode> FOptionsDialogNodes;
  QPointer<OptionsDialog> FOptionsDialog;
};

#endif // OPTIONSMANAGER_H
