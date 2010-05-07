#ifndef ISTANZAPROCESSOR_H
#define ISTANZAPROCESSOR_H

#include <QList>
#include <utils/jid.h>
#include <utils/stanza.h>

#define STANZAPROCESSOR_UUID  "{1175D470-5D4A-4c29-A69E-EDA46C2BC387}"

#define SHO_DEFAULT           1000

class IStanzaHandler
{
public:
	virtual QObject* instance() =0;
	virtual bool stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept) =0;
	virtual bool stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept) =0;
};

class IStanzaRequestOwner
{
public:
	virtual QObject* instance() =0;
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza) =0;
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId) =0;
};

struct IStanzaHandle
{
	enum Direction {
		DirectionIn,
		DirectionOut
	};
	IStanzaHandle() { order=SHO_DEFAULT; direction=DirectionIn; handler=NULL; }
	int order;
	int direction;
	Jid streamJid;
	IStanzaHandler *handler;
	QList<QString> conditions;
};

class IStanzaProcessor
{
public:
	virtual QObject *instance() =0;
	virtual QString newId() const =0;
	virtual bool sendStanzaIn(const Jid &AStreamJid, Stanza &AStanza) =0;
	virtual bool sendStanzaOut(const Jid &AStreamJid, Stanza &AStanza) =0;
	virtual bool sendStanzaRequest(IStanzaRequestOwner *AOwner, const Jid &AStreamJid, Stanza &AStanza, int ATimeout) =0;
	virtual QList<int> stanzaHandles() const =0;
	virtual IStanzaHandle stanzaHandle(int AHandleId) const =0;
	virtual int insertStanzaHandle(const IStanzaHandle &AHandle) =0;
	virtual void removeStanzaHandle(int AHandleId) =0;
	virtual bool checkStanza(const Stanza &AStanza, const QString &ACondition) const =0;
protected:
	virtual void stanzaSent(const Jid &AStreamJid, const Stanza &AStanza) =0;
	virtual void stanzaReceived(const Jid &AStreamJid, const Stanza &AStanza) =0;
	virtual void stanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle) =0;
	virtual void stanzaHandleRemoved(int AHandleId, const IStanzaHandle &AHandle) =0;
};

Q_DECLARE_INTERFACE(IStanzaHandler,"Vacuum.Plugin.IStanzaHandler/1.0");
Q_DECLARE_INTERFACE(IStanzaRequestOwner,"Vacuum.Plugin.IStanzaRequestOwner/1.0");
Q_DECLARE_INTERFACE(IStanzaProcessor,"Vacuum.Plugin.IStanzaProcessor/1.0");

#endif
