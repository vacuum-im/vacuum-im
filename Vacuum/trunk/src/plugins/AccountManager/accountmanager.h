#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QPointer>
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

#define ACCOUNTMANAGER_UUID "{56F1AA4C-37A6-4007-ACFE-557EEBD86AF8}"

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

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return ACCOUNTMANAGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();
  
  //IAccountManager
  virtual IAccount *addAccount(const QString &AName, const Jid &AStreamJid);
  virtual IAccount *addAccount(const QString &AAccountId, 
    const QString &AName = QString(), const Jid &AStreamJid= Jid());
  virtual void showAccount(IAccount *AAccount);
  virtual void hideAccount(IAccount *AAccount);
  virtual IAccount *accountById(const QString &AAcoountId) const;
  virtual IAccount *accountByName(const QString &AName) const;
  virtual IAccount *accountByStream(const Jid &AStreamJid) const;
  virtual void removeAccount(IAccount *AAccount);
  virtual void destroyAccount(const QString &AAccountId);

  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder) const;
signals:
  virtual void added(IAccount *);
  virtual void shown(IAccount *);
  virtual void hidden(IAccount *);
  virtual void removed(IAccount *);
  virtual void destroyed(IAccount *);
protected:
  QString newId() const;
  void openAccountOptionsNode(const QString &AAccountId, const QString &AName = QString());
  void closeAccountOptionsNode(const QString &AAccountId);
protected slots:
  void onOptionsAccountAdded(const QString &AName);
  void onOptionsAccountRemoved(const QString &AAccountId);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onSettingsOpened();
  void onSettingsClosed();
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
private:
  IPluginManager *FPluginManager;
  ISettingsPlugin *FSettingsPlugin;
  ISettings *FSettings;
  IXmppStreams *FXmppStreams;
  IMainWindowPlugin *FMainWindowPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  Action *FAccountsSetup;
private:
  mutable QPointer<AccountManage> FAccountManage;
  mutable QHash<QString,QPointer<AccountOptions> > FAccountOptions;
private:
  QList<Account *> FAccounts;
};

#endif // ACCOUNTMANAGER_H
