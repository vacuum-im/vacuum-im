#ifndef ADDRESS_H
#define ADDRESS_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/ipresencemanager.h>

class Address : 
	public QObject,
	public IMessageAddress
{
	Q_OBJECT;
	Q_INTERFACES(IMessageAddress);
public:
	Address(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid, QObject *AParent);
	~Address();
	virtual QObject *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual bool isAutoAddresses() const;
	virtual void setAutoAddresses(bool AEnabled);
	virtual QMultiMap<Jid, Jid> availAddresses(bool AUnique=false) const;
	virtual void setAddress(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void appendAddress(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void removeAddress(const Jid &AStreamJid, const Jid &AContactJid = Jid::null);
signals:
	void availAddressesChanged();
	void autoAddressesChanged(bool AEnabled);
	void addressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
protected:
	bool updateAutoAddresses(bool AEmit);
protected slots:
	void onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
private:
	IXmppStreamManager *FXmppStreamManager;
	IMessageWidgets *FMessageWidgets;
	IPresenceManager *FPresenceManager;
private:
	Jid FStreamJid;
	Jid FContactJid;
	bool FAutoAddresses;
	QMap<Jid, QMultiMap<Jid,Jid> > FAddresses;
};

#endif // ADDRESS_H
