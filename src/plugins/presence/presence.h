#ifndef PRESENCE_H
#define PRESENCE_H

#include <interfaces/ipresencemanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreammanager.h>

class Presence :
	public QObject,
	public IPresence,
	private IStanzaHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPresence IStanzaHandler);
public:
	Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
	~Presence();
	virtual QObject *instance() { return this; }
	//IStanzaProcessorHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IPresence
	virtual Jid streamJid() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool isOpen() const;
	virtual int show() const;
	virtual bool setShow(int AShow);
	virtual QString status() const;
	virtual bool setStatus(const QString &AStatus);
	virtual int priority() const;
	virtual bool setPriority(int APriority);
	virtual bool setPresence(int AShow, const QString &AStatus, int APriority);
	virtual bool sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	virtual QList<Jid> itemsJid() const;
	virtual QList<IPresenceItem> items() const;
	virtual IPresenceItem findItem(const Jid &AItemFullJid) const;
	virtual QList<IPresenceItem> findItems(const Jid &AItemBareJid) const;
signals:
	void opened();
	void closed();
	void aboutToClose(int AShow, const QString &AStatus);
	void changed(int AShow, const QString &AStatus, int APriority);
	void itemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void directSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void presenceDestroyed();
protected:
	void clearPresenceItems();
protected slots:
	void onXmppStreamError(const XmppError &AError);
	void onXmppStreamClosed();
private:
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FShow;
	int FPriority;
	QString FStatus;
private:
	bool FOpened;
	int FSHIPresence;
	QHash<Jid, QMap<QString, IPresenceItem> > FItems;
};

#endif // PRESENCE_H
