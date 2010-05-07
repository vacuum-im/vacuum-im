#ifndef PRESENCE_H
#define PRESENCE_H

#include <QObject>
#include <interfaces/ipresence.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>

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
	virtual bool stanzaEdit(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza & /*AStanza*/, bool &/*AAccept*/) { return false; }
	virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
	//IPresence
	virtual Jid streamJid() const { return FXmppStream->streamJid(); }
	virtual IXmppStream *xmppStream() const { return FXmppStream; }
	virtual bool isOpen() const { return FOpened; }
	virtual int show() const { return FShow; }
	virtual bool setShow(int AShow);
	virtual QString status() const { return FStatus; }
	virtual bool setStatus(const QString &AStatus);
	virtual int priority() const { return FPriority; }
	virtual bool setPriority(int APriority);
	virtual bool setPresence(int AShow, const QString &AStatus, int APriority);
	virtual bool sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	virtual IPresenceItem presenceItem(const Jid &AItemJid) const { return FItems.value(AItemJid); }
	virtual QList<IPresenceItem> presenceItems(const Jid &AItemJid = Jid()) const;
signals:
	void opened();
	void changed(int AShow, const QString &AStatus, int APriority);
	void received(const IPresenceItem &APresenceItem);
	void sent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void aboutToClose(int AShow, const QString &AStatus);
	void closed();
protected:
	void clearItems();
protected slots:
	void onStreamError(const QString &AError);
	void onStreamClosed();
private:
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
private:
	bool FOpened;
	int FSHIPresence;
	int FShow;
	int FPriority;
	QString FStatus;
	QHash<Jid, IPresenceItem> FItems;
};

#endif // PRESENCE_H
