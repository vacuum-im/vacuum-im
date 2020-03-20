#ifndef MULTIUSERCHAT_H
#define MULTIUSERCHAT_H

#include <interfaces/imultiuserchat.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresencemanager.h>
#include <utils/pluginhelper.h>
#include "multiuser.h"

class MultiUserChat :
	public QObject,
	public IMultiUserChat,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IMessageEditor
{
	Q_OBJECT;
	Q_INTERFACES(IMultiUserChat IStanzaHandler IStanzaRequestOwner IMessageEditor);
public:
	MultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANickName, const QString &APassword, bool AIsolated, QObject *AParent);
	~MultiUserChat();
	virtual QObject *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IIqStanzaOwnner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IMessageEditor
	virtual bool messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection);
	//IMultiUserChat
	virtual Jid streamJid() const;
	virtual Jid roomJid() const;
	virtual QString roomName() const;
	virtual QString roomTitle() const;
	virtual int state() const;
	virtual bool isOpen() const;
	virtual bool isIsolated() const;
	virtual bool autoPresence() const;
	virtual void setAutoPresence(bool AAuto);
	virtual QList<int> statusCodes() const;
	virtual QList<int> statusCodes(const Stanza &AStanza) const;
	virtual XmppError roomError() const;
	virtual IPresenceItem roomPresence() const;
	virtual IMultiUser *mainUser() const;
	virtual QList<IMultiUser *> allUsers() const;
	virtual IMultiUser *findUser(const QString &ANick) const;
	virtual IMultiUser *findUserByRealJid(const Jid &ARealJid) const;
	virtual bool isUserPresent(const Jid &AContactJid) const;
	virtual void abortConnection(const QString &AStatus, bool AError=true);
	//Occupant
	virtual QString nickname() const;
	virtual bool setNickname(const QString &ANick);
	virtual QString password() const;
	virtual void setPassword(const QString &APassword);
	virtual IMultiUserChatHistory historyScope() const;
	virtual void setHistoryScope(const IMultiUserChatHistory &AHistory);
	virtual bool sendStreamPresence();
	virtual bool sendPresence(int AShow, const QString &AStatus, int APriority);
	virtual bool sendMessage(const Message &AMessage, const QString &AToNick=QString());
	virtual bool sendInvitation(const QList<Jid> &AContacts, const QString &AReason=QString(), const QString &AThread=QString());
	virtual bool sendDirectInvitation(const QList<Jid> &AContacts, const QString &AReason=QString(), const QString &AThread=QString());
	virtual bool sendVoiceRequest();
	//Moderator
	virtual QString subject() const;
	virtual bool sendSubject(const QString &ASubject);
	virtual bool sendVoiceApproval(const Message &AMessage);
	//Administrator
	virtual QString loadAffiliationList(const QString &AAffiliation);
	virtual QString updateAffiliationList(const QList<IMultiUserListItem> &AItems);
	virtual QString setUserRole(const QString &ANick, const QString &ARole, const QString &AReason=QString());
	virtual QString setUserAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason=QString());
	//Owner
	virtual QString loadRoomConfig();
	virtual QString updateRoomConfig(const IDataForm &AForm);
	virtual QString destroyRoom(const QString &AReason);
signals:
	void stateChanged(int AState);
	void chatDestroyed();
	//Common
	void roomTitleChanged(const QString &ATitle);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void requestFailed(const QString &AId, const XmppError &AError);
	//Occupant
	void messageSent(const Message &AMessage);
	void messageReceived(const Message &AMessage);
	void passwordChanged(const QString &APassword);
	void presenceChanged(const IPresenceItem &APresence);
	void nicknameChanged(const QString &ANick, const XmppError &AError);
	void invitationSent(const QList<Jid> &AContacts, const QString &AReason, const QString &AThread);
	void invitationDeclined(const Jid &AContactJid, const QString &AReason);
	void invitationFailed(const QList<Jid> &AContacts, const XmppError &AError);
	void userChanged(IMultiUser *AUser, int AData, const QVariant &ABefore);
	//Moderator
	void voiceRequestReceived(const Message &AMessage);
	void subjectChanged(const QString &ANick, const QString &ASubject);
	void userKicked(const QString &ANick, const QString &AReason, const QString &AByUser);
	void userBanned(const QString &ANick, const QString &AReason, const QString &AByUser);
	//Administrator
	void userRoleUpdated(const QString &AId, const QString &ANick);
	void userAffiliationUpdated(const QString &AId, const QString &ANick);
	void affiliationListLoaded(const QString &AId, const QList<IMultiUserListItem> &AItems);
	void affiliationListUpdated(const QString &AId, const QList<IMultiUserListItem> &AItems);
	//Owner
	void roomConfigLoaded(const QString &AId, const IDataForm &AForm);
	void roomConfigUpdated(const QString &AId, const IDataForm &AForm);
	void roomDestroyed(const QString &AId, const QString &AReason);
protected:
	void setState(ChatState AState);
	bool processMessage(const Stanza &AStanza);
	bool processPresence(const Stanza &AStanza);
	void closeRoom(const IPresenceItem &APresence);
	Stanza makePresenceStanza(const QString &ANick, int AShow, const QString &AStatus, int APriority) const;
protected slots:
	void onUserChanged(int AData, const QVariant &ABefore);
	void onDiscoveryInfoReceived(const IDiscoInfo &AInfo);
protected slots:
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
private:
	PluginPointer<IDataForms> FDataForms;
	PluginPointer<IServiceDiscovery> FDiscovery;
	PluginPointer<IPresenceManager> FPresenceManager;
	PluginPointer<IStanzaProcessor> FStanzaProcessor;
	PluginPointer<IMessageProcessor> FMessageProcessor;
	PluginPointer<IXmppStreamManager> FXmppStreamManager;
private:
	int FSHIMessage;
	int FSHIPresence;
	QString FRequestedNick;
	QList<QString> FConfigLoadId;
	QMap<QString, IDataForm> FConfigUpdateId;
	QMap<QString, QString> FRoleUpdateId;
	QMap<QString, QString> FAffilUpdateId;
	QMap<QString, QString> FAffilListLoadId;
	QMap<QString, QList<IMultiUserListItem> > FAffilListUpdateId;
	QMap<QString, QString> FRoomDestroyId;
private:
	bool FIsolated;
	bool FAutoPresence;
	bool FResendPresence;
	Jid FStreamJid;
	Jid FRoomJid;
	ChatState FState;
	QString FSubject;
	QString FNickname;
	QString FPassword;
	QString FRoomTitle;
	MultiUser *FMainUser;
	XmppError FRoomError;
	IPresenceItem FRoomPresence;
	QList<int> FStatusCodes;
	IMultiUserChatHistory FHistory;
	QHash<QString, MultiUser *> FUsers;
};

#endif // MULTIUSERCHAT_H
