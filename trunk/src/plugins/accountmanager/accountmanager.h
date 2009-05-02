#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QPointer> 
#include "../../definations/actiongroups.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionnodeorders.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/accountvaluenames.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersview.h"
#include "../../utils/action.h"
#include "account.h"
#include "accountmanage.h"
#include "accountoptions.h"

class AccountManager :
  public QObject,
  public IPlugin,
  public IAccountManager,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IAccountManager IOptionsHolder);
public:
  AccountManager();
  ~AccountManager();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ACCOUNTMANAGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IAccountManager
  virtual IAccount *insertAccount(const QString &AAccountId = "");
  virtual QList<IAccount *> accounts() const;
  virtual IAccount *accountById(const QString &AAcoountId) const;
  virtual IAccount *accountByStream(const Jid &AStreamJid) const;
  virtual void showAccount(const QString &AAccountId);
  virtual void hideAccount(const QString &AAccountId);
  virtual void removeAccount(const QString &AAccountId);
  virtual void destroyAccount(const QString &AAccountId);
signals:
  virtual void inserted(IAccount *AAccount);
  virtual void shown(IAccount *AAccount);
  virtual void hidden(IAccount *AAccount);
  virtual void removed(IAccount *AAccount);
  virtual void destroyed(const QString &AAccountId);
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  QString newId() const;
  void openAccountOptionsNode(const QString &AAccountId, const QString &AName = "");
  void closeAccountOptionsNode(const QString &AAccountId);
protected slots:
  void onOptionsAccountAdded(const QString &AName);
  void onOptionsAccountShow(const QString &AAccountId);
  void onOptionsAccountRemoved(const QString &AAccountId);
  void onOpenOptionsDialogByAction(bool);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onOptionsDialogClosed();
  void onProfileOpened(const QString &AProfile);
  void onProfileClosed(const QString &AProfile);
  void onSettingsOpened();
  void onSettingsClosed();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
private:
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;
  IXmppStreams *FXmppStreams;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  AccountManage *FAccountSetup;
  QHash<QString, AccountOptions *> FAccountOptions;
private:
  QHash<QString, Account *> FAccounts;
};

#endif // ACCOUNTMANAGER_H
