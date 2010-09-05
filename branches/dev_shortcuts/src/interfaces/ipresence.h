#ifndef IPRESENCE_H
#define IPRESENCE_H

#include <QList>
#include <interfaces/ixmppstreams.h>
#include <utils/jid.h>

#define PRESENCE_UUID "{511A07C4-FFA4-43ce-93B0-8C50409AFC0E}"

struct IPresenceItem 
{
	IPresenceItem() { 
		isValid = false;
		show = 0;
		priority = 0;
	}
	bool isValid;
	Jid itemJid;
	int show;
	int priority;
	QString status;
};

class IPresence
{
public:
	enum Show {
		Offline,
		Online,
		Chat,
		Away,
		DoNotDisturb,
		ExtendedAway,
		Invisible,
		Error
	};
public:
	virtual QObject *instance() =0;
	virtual Jid streamJid() const =0;
	virtual IXmppStream *xmppStream() const =0;
	virtual bool isOpen() const =0;
	virtual int show() const =0;
	virtual bool setShow(int AShow) =0;
	virtual QString status() const =0;
	virtual bool setStatus(const QString &AStatus) =0;
	virtual int priority() const =0;
	virtual bool setPriority(int APriority) =0;
	virtual bool setPresence(int AShow, const QString &AStatus, int APriority) =0;
	virtual bool sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority) =0;
	virtual IPresenceItem presenceItem(const Jid &AItemJid) const =0;
	virtual QList<IPresenceItem> presenceItems(const Jid &AItemJid = Jid()) const =0;
protected:
	virtual void opened() =0;
	virtual void changed(int AShow, const QString &AStatus, int APriority) =0;
	virtual void received(const IPresenceItem &APresenceItem) =0;
	virtual void sent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority) =0;
	virtual void aboutToClose(int AShow, const QString &AStatus) =0;
	virtual void closed() =0;
};

class IPresencePlugin
{
public:
	virtual QObject *instance() =0;
	virtual IPresence *addPresence(IXmppStream *AXmppStream) =0;
	virtual IPresence *getPresence(const Jid &AStreamJid) const =0;
	virtual bool isContactOnline(const Jid &AContactJid) const =0;
	virtual QList<Jid> contactsOnline() const =0;
	virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const =0;
	virtual void removePresence(IXmppStream *AXmppStream) =0;
protected:
	virtual void streamStateChanged(const Jid &AStreamJid, bool AStateOnline) =0;
	virtual void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline) =0;
	virtual void presenceAdded(IPresence *APresence) =0;
	virtual void presenceOpened(IPresence *APresence) =0;
	virtual void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority) =0;
	virtual void presenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem) =0;
	virtual void presenceSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity) =0;
	virtual void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus) =0;
	virtual void presenceClosed(IPresence *APresence) =0;
	virtual void presenceRemoved(IPresence *APresence) =0;
};

Q_DECLARE_INTERFACE(IPresence,"Vacuum.Plugin.IPresence/1.0")
Q_DECLARE_INTERFACE(IPresencePlugin,"Vacuum.Plugin.IPresencePlugin/1.0")

#endif  //IPRESENCE_H
