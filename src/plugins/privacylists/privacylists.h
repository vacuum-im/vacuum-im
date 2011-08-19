#ifndef PRIVACYLISTS_H
#define PRIVACYLISTS_H

#include <QTimer>
#include <definitions/namespaces.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iprivacylists.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include "editlistsdialog.h"

class PrivacyLists :
			public QObject,
			public IPlugin,
			public IPrivacyLists,
			public IStanzaHandler,
			public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPrivacyLists IStanzaHandler IStanzaRequestOwner);
public:
	PrivacyLists();
	~PrivacyLists();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRIVACYLISTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IPrivacyLists
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual IPrivacyRule autoListRule(const Jid &AContactJid, const QString &AAutoList) const;
	virtual IPrivacyRule autoListRule(const QString &AGroup, const QString &AAutoList) const;
	virtual bool isAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList) const;
	virtual bool isAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AList) const;
	virtual void setAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList, bool AInserted);
	virtual void setAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AList, bool AInserted);
	virtual IPrivacyRule offRosterRule() const;
	virtual bool isOffRosterBlocked(const Jid &AStreamJid) const;
	virtual void setOffRosterBlocked(const Jid &AStreamJid, bool ABlocked);
	virtual bool isAutoPrivacy(const Jid &AStreamJid) const;
	virtual void setAutoPrivacy(const Jid &AStreamJid, const QString &AAutoList);
	virtual int denyedStanzas(const IRosterItem &AItem, const IPrivacyList &AList) const;
	virtual QHash<Jid,int> denyedContacts(const Jid &AStreamJid, const IPrivacyList &AList, int AFilter=IPrivacyRule::AnyStanza) const;
	virtual QString activeList(const Jid &AStreamJid, bool APending = false) const;
	virtual QString setActiveList(const Jid &AStreamJid, const QString &AList);
	virtual QString defaultList(const Jid &AStreamJid, bool APending = false) const;
	virtual QString setDefaultList(const Jid &AStreamJid, const QString &AList);
	virtual IPrivacyList privacyList(const Jid &AStreamJid, const QString &AList, bool APending = false) const;
	virtual QList<IPrivacyList> privacyLists(const Jid &AStreamJid, bool APending = false) const;
	virtual QString loadPrivacyList(const Jid &AStreamJid, const QString &AList);
	virtual QString savePrivacyList(const Jid &AStreamJid, const IPrivacyList &AList);
	virtual QString removePrivacyList(const Jid &AStreamJid, const QString &AList);
	virtual QDialog *showEditListsDialog(const Jid &AStreamJid, QWidget *AParent = NULL);
signals:
	void listAboutToBeChanged(const Jid &AStreamJid, const IPrivacyList &AList);
	void listLoaded(const Jid &AStreamJid, const QString &AList);
	void listRemoved(const Jid &AStreamJid, const QString &AList);
	void activeListAboutToBeChanged(const Jid &AStreamJid, const QString &AList);
	void activeListChanged(const Jid &AStreamJid, const QString &AList);
	void defaultListChanged(const Jid &AStreamJid, const QString &AList);
	void requestCompleted(const QString &AId);
	void requestFailed(const QString &AId, const QString &AError);
protected:
	QString loadPrivacyLists(const Jid &AStreamJid);
	Menu *createPrivacyMenu(Menu *AMenu) const;
	void createAutoPrivacyStreamActions(const Jid &AStreamJid, Menu *AMenu) const;
	void createAutoPrivacyContactActions(const Jid &AStreamJid, const Jid &AContactJid, Menu *AMenu) const;
	void createAutoPrivacyGroupActions(const Jid &AStreamJid, const QString &AGroup, Menu *AMenu) const;
	Menu *createSetActiveMenu(const Jid &AStreamJid, const QList<IPrivacyList> &ALists, Menu *AMenu) const;
	Menu *createSetDefaultMenu(const Jid &AStreamJid, const QList<IPrivacyList> &ALists, Menu *AMenu) const;
	bool isMatchedJid(const Jid &AMask, const Jid &AJid) const;
	void sendOnlinePresences(const Jid &AStreamJid, const IPrivacyList &AAutoList);
	void sendOfflinePresences(const Jid &AStreamJid, const IPrivacyList &AAutoList);
	void setPrivacyLabel(const Jid &AStreamJid, const Jid &AContactJid, bool AVisible);
	void updatePrivacyLabels(const Jid &AStreamJid);
protected slots:
	void onListAboutToBeChanged(const Jid &AStreamJid, const IPrivacyList &AList);
	void onListChanged(const Jid &AStreamJid, const QString &AList);
	void onActiveListAboutToBeChanged(const Jid &AStreamJid, const QString &AList);
	void onActiveListChanged(const Jid &AStreamJid, const QString &AList);
	void onApplyAutoLists();
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onRosterIndexCreated(IRosterIndex *AIndex, IRosterIndex *AParent);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void onShowEditListsDialog(bool);
	void onSetActiveListByAction(bool);
	void onSetDefaultListByAction(bool);
	void onSetAutoPrivacyByAction(bool);
	void onChangeContactAutoListed(bool AInserted);
	void onChangeGroupAutoListed(bool AInserted);
	void onChangeOffRosterBlocked(bool ABlocked);
	void onEditListsDialogDestroyed(const Jid AStreamJid);
	void onMultiUserChatCreated(IMultiUserChat *AMultiChat);
private:
	IXmppStreams *FXmppStreams;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
private:
	int FRosterLabelId;
	QTimer FApplyAutoListsTimer;
	QHash<Jid,int> FSHIPrivacy;
	QHash<Jid,int> FSHIRosterIn;
	QHash<Jid,int> FSHIRosterOut;
	QHash<QString, IPrivacyList> FSaveRequests;
	QHash<QString, QString> FLoadRequests;
	QHash<QString, QString> FActiveRequests;
	QHash<QString, QString> FDefaultRequests;
	QHash<QString, QString> FRemoveRequests;
	QHash<Jid, QStringList > FStreamRequests;
	QHash<Jid, QString> FApplyAutoLists;
	QHash<Jid, QSet<Jid> > FOfflinePresences;
	QHash<Jid, QString> FActiveLists;
	QHash<Jid, QString> FDefaultLists;
	QHash<Jid, QHash<QString,IPrivacyList> > FPrivacyLists;
	QHash<Jid, QSet<Jid> > FLabeledContacts;
	QHash<Jid, EditListsDialog *> FEditListsDialogs;
private:
	static QStringList FAutoLists;
};

#endif // PRIVACYLISTS_H
