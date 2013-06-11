#ifndef ROSTER_H
#define ROSTER_H

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/iroster.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <utils/options.h>
#include <utils/xmpperror.h>

class Roster :
	public QObject,
	public IRoster,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IRoster IStanzaHandler IStanzaRequestOwner IXmppStanzaHadler);
public:
	Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
	~Roster();
	virtual QObject *instance() { return this; }
	//IStanzaProcessorHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaProcessorIqOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IRoster
	virtual Jid streamJid() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool isOpen() const;
	virtual QString groupDelimiter() const;
	virtual IRosterItem rosterItem(const Jid &AItemJid) const;
	virtual QList<IRosterItem> rosterItems() const;
	virtual QSet<QString> allGroups() const;
	virtual bool hasGroup(const QString &AGroup) const;
	virtual QSet<QString> itemGroups(const Jid &AItemJid) const;
	virtual QList<IRosterItem> groupItems(const QString &AGroup) const;
	virtual bool isSubgroup(const QString &ASubGroup, const QString &AGroup) const;
	virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups);
	virtual void setItems(const QList<IRosterItem> &AItems);
	virtual void removeItem(const Jid &AItemJid);
	virtual void removeItems(const QList<IRosterItem> &AItems);
	virtual void saveRosterItems(const QString &AFileName) const;
	virtual void loadRosterItems(const QString &AFileName);
	//Operations  on subscription
	virtual QSet<Jid> subscriptionRequests() const;
	virtual void sendSubscription(const Jid &AItemJid, int AType, const QString &AText = QString::null);
	//Operations on items
	virtual void renameItem(const Jid &AItemJid, const QString &AName);
	virtual void copyItemToGroup(const Jid &AItemJid, const QString &AGroupTo);
	virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo);
	virtual void removeItemFromGroup(const Jid &AItemJid, const QString &AGroupFrom);
	//Operations on group
	virtual void renameGroup(const QString &AGroup, const QString &AGroupTo);
	virtual void copyGroupToGroup(const QString &AGroup, const QString &AGroupTo);
	virtual void moveGroupToGroup(const QString &AGroup, const QString &AGroupTo);
	virtual void removeGroup(const QString &AGroup);
signals:
	void opened();
	void itemReceived(const IRosterItem &ARosterItem, const IRosterItem &ABefore);
	void subscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText);
	void subscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText);
	void closed();
	void streamJidAboutToBeChanged(const Jid &AAfter);
	void streamJidChanged(const Jid &ABefore);
protected:
	void clearRosterItems();
	void requestRosterItems();
	void requestGroupDelimiter();
	void setGroupDelimiter(const QString &ADelimiter);
	void processItemsElement(const QDomElement &AItemsElem, bool ACompleteRoster);
	QString replaceGroupDelimiter(const QString &AGroup, const QString &AFrom, const QString &ATo) const;
protected slots:
	void onStreamOpened();
	void onStreamClosed();
	void onStreamJidAboutToBeChanged(const Jid &AAfter);
	void onStreamJidChanged(const Jid &ABefore);
private:
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHIRosterPush;
	int FSHISubscription;
	QString FOpenRequestId;
	QString FDelimRequestId;
private:
	bool FOpened;
	bool FVerSupported;
	QString FRosterVer;
	QString FGroupDelim;
	QSet<Jid> FSubscriptionRequests;
	QHash<Jid, IRosterItem> FRosterItems;
};

#endif //ROSTER_H
