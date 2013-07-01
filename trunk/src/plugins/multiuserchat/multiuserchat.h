#ifndef MULTIUSERCHAT_H
#define MULTIUSERCHAT_H

#include <definitions/multiuserdataroles.h>
#include <definitions/namespaces.h>
#include <definitions/messageeditororders.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresence.h>
#include <utils/xmpperror.h>
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
	MultiUserChat(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANickName, const QString &APassword, QObject *AParent);
	~MultiUserChat();
	virtual QObject *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IIqStanzaOwnner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IMessageEditor
	virtual bool messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection);
	//IMultiUserChar
	virtual Jid streamJid() const;
	virtual Jid roomJid() const;
	virtual QString roomName() const;
	virtual bool isOpen() const;
	virtual bool isConnected() const;
	virtual bool autoPresence() const;
	virtual void setAutoPresence(bool AAuto);
	virtual QList<int> statusCodes() const;
	virtual bool isUserPresent(const Jid &AContactJid) const;
	virtual IMultiUser *mainUser() const;
	virtual QList<IMultiUser *> allUsers() const;
	virtual IMultiUser *userByNick(const QString &ANick) const;
	//Occupant
	virtual QString nickName() const;
	virtual bool setNickName(const QString &ANick);
	virtual QString password() const;
	virtual void setPassword(const QString &APassword);
	virtual IMultiUserChatHistory history() const;
	virtual void setHistory(const IMultiUserChatHistory &AHistory);
	virtual int show() const;
	virtual QString status() const;
	virtual XmppError roomError() const;
	virtual bool sendStreamPresence();
	virtual bool sendPresence(int AShow, const QString &AStatus);
	virtual bool sendMessage(const Message &AMessage, const QString &AToNick = QString::null);
	virtual bool requestVoice();
	virtual bool inviteContact(const Jid &AContactJid, const QString &AReason);
	//Moderator
	virtual QString subject() const;
	virtual bool sendSubject(const QString &ASubject);
	virtual bool sendDataFormMessage(const IDataForm &AForm);
	//Administrator
	virtual bool setRole(const QString &ANick, const QString &ARole, const QString &AReason = QString::null);
	virtual bool setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason = QString::null);
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
	void roomNameChanged(const QString &AName);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
	//Occupant
	void userPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void userDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefore, const QVariant &AAfter);
	void userNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
	void presenceChanged(int AShow, const QString &AStatus);
	void serviceMessageReceived(const Message &AMessage);
	void messageSent(const Message &AMessage);
	void messageReceived(const QString &ANick, const Message &AMessage);
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
	void configFormRejected(const XmppError &AError);
	void roomDestroyed(const QString &AReason);
protected:
	void initialize();
	bool processMessage(const Stanza &AStanza);
	bool processPresence(const Stanza &AStanza);
	void closeChat(int AShow, const QString &AStatus);
protected slots:
	void onUserDataChanged(int ARole, const QVariant &ABefore, const QVariant &AAfter);
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onDiscoveryInfoReceived(const IDiscoInfo &AInfo);
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onStreamClosed();
	void onStreamJidChanged(const Jid &ABefore);
private:
	IPresence *FPresence;
	IDataForms *FDataForms;
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatPlugin *FChatPlugin;
	IMessageProcessor *FMessageProcessor;
	IServiceDiscovery *FDiscovery;
private:
	QString FRoomName;
	QString FConfigRequestId;
	QString FConfigSubmitId;
	QMap<QString, QString> FAffilListRequests;
	QMap<QString, QString> FAffilListSubmits;
private:
	int FSHIPresence;
	int FSHIMessage;
	int FShow;
	int FErrorCode;
	bool FConnected;
	bool FAutoPresence;
	Jid FStreamJid;
	Jid FRoomJid;
	QString FStatus;
	QString FSubject;
	QString FNickName;
	QString FPassword;
	MultiUser *FMainUser;
	XmppError FRoomError;
	QList<int> FStatusCodes;
	IMultiUserChatHistory FHistory;
	QHash<QString, MultiUser *> FUsers;
};

#endif // MULTIUSERCHAT_H
