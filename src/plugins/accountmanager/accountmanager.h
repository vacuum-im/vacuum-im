#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QComboBox>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/irostersview.h>

class AccountsOptionsWidget;

class AccountManager :
	public QObject,
	public IPlugin,
	public IAccountManager,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAccountManager IOptionsDialogHolder);
public:
	AccountManager();
	~AccountManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ACCOUNTMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IAccountManager
	virtual QList<IAccount *> accounts() const;
	virtual IAccount *findAccountById(const QUuid &AAcoountId) const;
	virtual IAccount *findAccountByStream(const Jid &AStreamJid) const;
	virtual IAccount *createAccount(const Jid &AAccountJid, const QString &AName);
	virtual void destroyAccount(const QUuid &AAccountId);
signals:
	void accountInserted(IAccount *AAccount);
	void accountRemoved(IAccount *AAccount);
	void accountDestroyed(const QUuid &AAccountId);
	void accountActiveChanged(IAccount *AAccount, bool AActive);
	void accountOptionsChanged(IAccount *AAcount, const OptionsNode &ANode);
protected:
	IAccount *insertAccount(const OptionsNode &AOptions);
	void removeAccount(const QUuid &AAccountId);
protected:
	void openAccountOptionsNode(const QUuid &AAccountId);
	void closeAccountOptionsNode(const QUuid &AAccountId);
	void showAccountOptionsDialog(const QUuid &AAccountId, QWidget *AParent=NULL);
	QComboBox *newResourceComboBox(const QUuid &AAccountId, QWidget *AParent) const;
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onProfileOpened(const QString &AProfile);
	void onProfileClosed(const QString &AProfile);
protected slots:
	void onAccountActiveChanged(bool AActive);
	void onAccountOptionsChanged(const OptionsNode &ANode);
protected slots:
	void onShowAccountOptions(bool);
	void onShowCreateAccountWizard();
	void onResourceComboBoxEditFinished();
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
private:
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
	IXmppStreamManager *FXmppStreamManager;
private:
	QMap<QUuid, IAccount *> FAccounts;
};

#endif // ACCOUNTMANAGER_H
