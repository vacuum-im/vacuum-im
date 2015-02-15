#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QPointer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppstreams.h>
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
	void changed(IAccount *AAcount, const OptionsNode &ANode);
	void destroyed(const QUuid &AAccountId);
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
	void onShowAccountOptions(bool);
	void onAccountActiveChanged(bool AActive);
	void onAccountOptionsChanged(const OptionsNode &ANode);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onResourceComboBoxEditFinished();
private:
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QMap<QUuid, IAccount *> FAccounts;
};

#endif // ACCOUNTMANAGER_H
