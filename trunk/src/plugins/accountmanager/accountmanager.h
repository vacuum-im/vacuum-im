#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QPointer> 
#include <definations/actiongroups.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/accountvaluenames.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isettings.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <utils/action.h>
#include "account.h"
#include "accountsoptions.h"

class AccountsOptions;

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
  virtual QList<IAccount *> accounts() const;
  virtual IAccount *accountById(const QUuid &AAcoountId) const;
  virtual IAccount *accountByStream(const Jid &AStreamJid) const;
  virtual IAccount *appendAccount(const QUuid &AAccountId);
  virtual void showAccount(const QUuid &AAccountId);
  virtual void hideAccount(const QUuid &AAccountId);
  virtual void removeAccount(const QUuid &AAccountId);
  virtual void destroyAccount(const QUuid &AAccountId);
signals:
  void appended(IAccount *AAccount);
  void shown(IAccount *AAccount);
  void hidden(IAccount *AAccount);
  void removed(IAccount *AAccount);
  void destroyed(const QUuid &AAccountId);
  //IOptionsHolder
  void optionsAccepted();
  void optionsRejected();
public:
  void openAccountOptionsDialog(const QUuid &AAccountId);
  void openAccountOptionsNode(const QUuid &AAccountId, const QString &AName);
  void closeAccountOptionsNode(const QUuid &AAccountId);
protected slots:
  void onAccountChanged(const QString &AName, const QVariant &AValue);
  void onOpenAccountOptions(bool);
  void onProfileOpened(const QString &AProfile);
  void onProfileClosed(const QString &AProfile);
  void onSettingsOpened();
  void onSettingsClosed();
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
private:
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;
  IXmppStreams *FXmppStreams;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  QMap<QUuid, IAccount *> FAccounts;
  QPointer<AccountsOptions> FAccountsOptions;
};

#endif // ACCOUNTMANAGER_H
