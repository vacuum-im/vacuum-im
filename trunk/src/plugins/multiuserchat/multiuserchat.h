#ifndef MULTIUSERCHAT_H
#define MULTIUSERCHAT_H

#include <definitions/multiuserdataroles.h>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>
#include "multiuser.h"

class MultiUserChat :
			public QObject,
			public IMultiUserChat,
			public IStanzaHandler,
			public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IMultiUserChat IStanzaHandler IStanzaRequestOwner);
public:
	MultiUserChat(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid,
	              const QString &ANickName, const QString &APassword, QObject *AParent);
	~MultiUserChat();
	virtual QObject *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
	//IIqStanzaOwnner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IMultiUserChar
	virtual Jid streamJid() const { return FStreamJid; }
	virtual Jid roomJid() const { return FRoomJid; }
	virtual bool isOpen() const;
	virtual bool autoPresence() const { return FAutoPresence; }
	virtual void setAutoPresence(bool AAuto);
	virtual QList<int> statusCodes() const { return FStatusCodes; }
	virtual bool isUserPresent(const Jid &AContactJid) const;
	virtual IMultiUser *mainUser() const { return FMainUser; }
	virtual IMultiUser *userByNick(const QString &ANick) const;
	virtual QList<IMultiUser *> allUsers() const;
	//Occupant
	virtual QString nickName() const { return FNickName; }
	virtual void setNickName(const QString &ANick);
	virtual QString password() const { return FPassword; }
	virtual void setPassword(const QString &APassword);
	virtual int show() const { return FShow; }
	virtual QString status() const { return FStatus; }
	virtual void setPresence(int AShow, const QString &AStatus);
	virtual bool sendMessage(const Message &AMessage, const QString &AToNick = "");
	virtual bool requestVoice();
	virtual bool inviteContact(const Jid &AContactJid, const QString &AReason);
	//Moderator
	virtual QString subject() const { return FSubject; }
	virtual void setSubject(const QString &ASubject);
	virtual void sendDataFormMessage(const IDataForm &AForm);
	//Administrator
	virtual void setRole(const QString &ANick, const QString &ARole, const QString &AReason = "");
	virtual void setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason = "");
	virtual bool requestAffiliationList(const QString &AAffiliation);
	virtual bool changeAffiliationList(const QList<IMultiUserListItem> &ADeltaList);
	//Owner
	virtual bool requestConfigForm();
	virtual bool sendConfigForm(const IDataForm &AForm);
	virtual bool destroyRoom(const QString &AReason);
signals:
	void chatOpened();
	void chatNotify(const QString &ANotify);
	void chatError(const QString &AMessage);
	void chatClosed();
	void chatDestroyed();
	void streamJidChanged(const Jid &ABefour, const Jid &AAfter);
	//Occupant
	void userPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void userDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter);
	void userNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
	void presenceChanged(int AShow, const QString &AStatus);
	void serviceMessageReceived(const Message &AMessage);
	void messageReceive(const QString &ANick, Message &AMessage);
	void messageReceived(const QString &ANick, const Message &AMessage);
	void messageSend(Message &AMessage);
	void messageSent(const Message &AMessage);
	void inviteDeclined(const Jid &AContactJid, const QString &AReason);
	//Moderator
	void subjectChanged(const QString &ANick, const QString &ASubject);
	void userKicked(const QString &ANick, const QString &AReason, const QString &AByUser);
	void dataFormMessageReceived(const Message &AMessage);
	void dataFormMessageSent(const IDataForm &AForm);
	//Administrator
	void userBanned(const QString &ANick, const QString &AReason, const QString &AByUser);
	void affiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList);
	void affiliationListChanged(const QList<IMultiUserListItem> &ADeltaList);
	//Owner
	void configFormReceived(const IDataForm &AForm);
	void configFormSent(const IDataForm &AForm);
	void configFormAccepted();
	void configFormRejected(const QString &AError);
	void roomDestroyed(const QString &AReason);
protected:
	void prepareMessageForReceive(Message &AMessage);
	bool processMessage(const Stanza &AStanza);
	bool processPresence(const Stanza &AStanza);
	void initialize();
	void closeChat(int AShow, const QString &AStatus);
protected slots:
	void onMessageReceive(Message &AMessage);
	void onMessageReceived(const Message &AMessage);
	void onMessageSend(Message &AMessage);
	void onMessageSent(const Message &AMessage);
	void onUserDataChanged(int ARole, const QVariant &ABefour, const QVariant &AAfter);
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onStreamClosed();
	void onStreamJidChanged(const Jid &ABefour);
private:
	IPresence *FPresence;
	IDataForms *FDataForms;
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatPlugin *FChatPlugin;
	IMessageProcessor *FMessageProcessor;
private:
	QString FConfigRequestId;
	QString FConfigSubmitId;
	QMap<QString, QString> FAffilListRequests;
	QMap<QString, QString> FAffilListSubmits;
private:
	int FSHIPresence;
	int FSHIMessage;
	bool FAutoPresence;
	Jid FStreamJid;
	Jid FRoomJid;
	int FShow;
	QString FStatus;
	QString FSubject;
	QString FNickName;
	QString FPassword;
	MultiUser *FMainUser;
	QList<int> FStatusCodes;
	QHash<QString, MultiUser *> FUsers;
};

#endif // MULTIUSERCHAT_H
