#ifndef STATUSICONS_H
#define STATUSICONS_H

#include <QRegExp>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ioptionsmanager.h>

class StatusIcons :
	public QObject,
	public IPlugin,
	public IStatusIcons,
	public IOptionsDialogHolder,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStatusIcons IOptionsDialogHolder IRosterDataHolder);
public:
	StatusIcons();
	~StatusIcons();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STATUSICONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IStatusIcons
	virtual QList<QString> rules(RuleType ARuleType) const;
	virtual QString ruleIconset(const QString &APattern, RuleType ARuleType) const;
	virtual void insertRule(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
	virtual void removeRule(const QString &APattern, RuleType ARuleType);
	virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QIcon iconByJidStatus(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QString iconsetByJid(const Jid &AContactJid) const;
	virtual QString iconKeyByJid(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString iconKeyByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QString iconFileName(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString iconFileName(const QString &ASubStorage, const QString &AIconKey) const;
signals:
	void defaultIconsetChanged(const QString &ASubStorage);
	void ruleInserted(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
	void ruleRemoved(const QString &APattern, RuleType ARuleType);
	void defaultIconsChanged();
	void statusIconsChanged();
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void loadStorages();
	void clearStorages();
	void startStatusIconsUpdate();
	void updateCustomIconMenu(const QStringList &APatterns);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onUpdateStatusIcons();
	void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onDefaultIconsetChanged();
	void onSetCustomIconsetByAction(bool);
private:
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IMultiUserChatManager *FMultiChatManager;
	IOptionsManager *FOptionsManager;
private:
	Menu *FCustomIconMenu;
	Action *FDefaultIconAction;
	IconStorage *FDefaultStorage;
	QHash<QString, Action *> FCustomIconActions;
private:
	bool FStatusIconsUpdateStarted;
	QSet<QString> FStatusRules;
	QMap<QString, QString> FUserRules;
	QMap<QString, QString> FDefaultRules;
	QMap<QString, IconStorage *> FStorages;
	mutable QHash<Jid, QString> FJid2Storage;
};

#endif // STATUSICONS_H
