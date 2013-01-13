#ifndef IMESSAGECARBOMS_H
#define IMESSAGECARBOMS_H

#include <QString>
#include <utils/jid.h>
#include <utils/message.h>
#include <utils/xmpperror.h>

#define MESSAGECARBONS_UUID "{C1F8895F-9DF4-4e95-8780-C2D88A520808}"

class IMessageCarbons
{
public:
	virtual QObject *instance() = 0;
	virtual bool isSupported(const Jid &AStreamJid) const =0;
	virtual bool isEnabled(const Jid &AStreamJid) const =0;
	virtual bool setEnabled(const Jid &AStreamJid, bool AEnabled) =0;
protected:
	virtual void enableChanged(const Jid &AStreamJid, bool AEnable) =0;
	virtual void messageSent(const Jid &AStreamJid, const Message &AMessage) =0;
	virtual void messageReceived(const Jid &AStreamJid, const Message &AMessage) =0;
	virtual void errorReceived(const Jid &AStreamJid, const XmppStanzaError &AError) =0;
};

Q_DECLARE_INTERFACE(IMessageCarbons,"Vacuum.Plugin.IMessageCarbons/1.0")

#endif // IMESSAGECARBOMS_H
