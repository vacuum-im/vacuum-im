#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>

class Account :
	public QObject,
	public IAccount
{
	Q_OBJECT;
	Q_INTERFACES(IAccount);
public:
	Account(IXmppStreams *AXmppStreams, const OptionsNode &AOptionsNode, QObject *AParent);
	~Account();
	virtual QObject *instance() { return this; }
	virtual bool isValid() const;
	virtual QUuid accountId() const;
	virtual bool isActive() const;
	virtual void setActive(bool AActive);
	virtual QString name() const;
	virtual void setName(const QString &AName);
	virtual Jid streamJid() const;
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual QString resource() const;
	virtual void setResource(const QString &AResource);
	virtual QString password() const;
	virtual void setPassword(const QString &APassword);
	virtual OptionsNode optionsNode() const;
	virtual IXmppStream *xmppStream() const;
signals:
	void activeChanged(bool AActive);
	void optionsChanged(const OptionsNode &ANode);
protected slots:
	void onXmppStreamClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IXmppStream *FXmppStream;
	IXmppStreams *FXmppStreams;
private:
	OptionsNode FOptionsNode;
};

#endif // ACCOUNT_H
