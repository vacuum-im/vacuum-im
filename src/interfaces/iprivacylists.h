#ifndef IPRIVACYLISTS_H
#define IPRIVACYLISTS_H

#include <QDialog>
#include <interfaces/iroster.h>
#include <utils/jid.h>
#include <utils/xmpperror.h>

#define PRIVACYLISTS_UUID "{B7B7F21A-DF0E-4f3e-B0C2-AA14976B546F}"

#define PRIVACY_TYPE_JID              "jid"
#define PRIVACY_TYPE_GROUP            "group"
#define PRIVACY_TYPE_SUBSCRIPTION     "subscription"
#define PRIVACY_TYPE_ALWAYS           ""

#define PRIVACY_ACTION_ALLOW          "allow"
#define PRIVACY_ACTION_DENY           "deny"

#define PRIVACY_LIST_VISIBLE          "visible-list"
#define PRIVACY_LIST_IGNORE           "ignore-list"
#define PRIVACY_LIST_INVISIBLE        "invisible-list"
#define PRIVACY_LIST_CONFERENCES      "conference-list"
#define PRIVACY_LIST_SUBSCRIPTION     "subscription-list"
#define PRIVACY_LIST_AUTO_VISIBLE     "i-am-visible-list"
#define PRIVACY_LIST_AUTO_INVISIBLE   "i-am-invisible-list"

struct IPrivacyRule
{
	enum StanzaType {
		EmptyType     =0,
		Messages      =1,
		Queries       =2,
		PresencesIn   =4,
		PresencesOut  =8,
		AnyStanza     = Messages|Queries|PresencesIn|PresencesOut
	};
	IPrivacyRule() { 
		stanzas = EmptyType; 
	}
	int order;
	QString type;
	QString value;
	QString action;
	int stanzas;
	bool operator<(const IPrivacyRule &ARule) const {
		return order < ARule.order;
	}
	bool operator==(const IPrivacyRule &ARule) const {
		return type==ARule.type && value==ARule.value && action==ARule.action && stanzas==ARule.stanzas;
	}
};

struct IPrivacyList
{
	QString name;
	QList<IPrivacyRule> rules;
	bool operator==(const IPrivacyList &AList) const {
		return name==AList.name && rules==AList.rules;
	}
	bool operator!=(const IPrivacyList &AList) const {
		return name!=AList.name || rules!=AList.rules;
	}
};

class IPrivacyLists
{
public:
	virtual QObject *instance() =0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual IPrivacyRule groupAutoListRule(const QString &AGroup, const QString &AAutoList) const =0;
	virtual IPrivacyRule contactAutoListRule(const Jid &AContactJid, const QString &AAutoList) const =0;
	virtual bool isGroupAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AList) const =0;
	virtual bool isContactAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList) const =0;
	virtual void setGroupAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AList, bool APresent) =0;
	virtual void setContactAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList, bool APresent) =0;
	virtual IPrivacyRule offRosterRule() const =0;
	virtual bool isOffRosterBlocked(const Jid &AStreamJid) const =0;
	virtual void setOffRosterBlocked(const Jid &AStreamJid, bool ABlocked) =0;
	virtual bool isAutoPrivacy(const Jid &AStreamJid) const =0;
	virtual void setAutoPrivacy(const Jid &AStreamJid, const QString &AAutoList) =0;
	virtual int denyedStanzas(const IRosterItem &AItem, const IPrivacyList &AList) const =0;
	virtual QHash<Jid,int> denyedContacts(const Jid &AStreamJid, const IPrivacyList &AList, int AFilter=IPrivacyRule::AnyStanza) const =0;
	virtual QString activeList(const Jid &AStreamJid, bool APending = false) const =0;
	virtual QString setActiveList(const Jid &AStreamJid, const QString &AList) =0;
	virtual QString defaultList(const Jid &AStreamJid, bool APending = false) const =0;
	virtual QString setDefaultList(const Jid &AStreamJid, const QString &AList) =0;
	virtual IPrivacyList privacyList(const Jid &AStreamJid, const QString &AList, bool APending = false) const =0;
	virtual QList<IPrivacyList> privacyLists(const Jid &AStreamJid, bool APending = false) const =0;
	virtual QString loadPrivacyList(const Jid &AStreamJid, const QString &AList) =0;
	virtual QString savePrivacyList(const Jid &AStreamJid, const IPrivacyList &AList) =0;
	virtual QString removePrivacyList(const Jid &AStreamJid, const QString &AList) =0;
	virtual QDialog *showEditListsDialog(const Jid &AStreamJid, QWidget *AParent = NULL) =0;
protected:
	virtual void privacyOpened(const Jid &AStreamJid) =0;
	virtual void privacyClosed(const Jid &AStreamJid) =0;
	virtual void listLoaded(const Jid &AStreamJid, const QString &AList) =0;
	virtual void listRemoved(const Jid &AStreamJid, const QString &AList) =0;
	virtual void listAboutToBeChanged(const Jid &AStreamJid, const IPrivacyList &AList) =0;
	virtual void activeListAboutToBeChanged(const Jid &AStreamJid, const QString &AList) =0;
	virtual void activeListChanged(const Jid &AStreamJid, const QString &AList) =0;
	virtual void defaultListChanged(const Jid &AStreamJid, const QString &AList) =0;
	virtual void requestCompleted(const QString &AId) =0;
	virtual void requestFailed(const QString &AId, const XmppError &AError) =0;
};

Q_DECLARE_INTERFACE(IPrivacyLists,"Vacuum.Plugin.IPrivacyLists/1.3")

#endif
