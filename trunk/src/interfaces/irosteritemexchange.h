#ifndef IROSTERITEMEXCHANGE_H
#define IROSTERITEMEXCHANGE_H

#include <QSet>
#include <utils/jid.h>
#include <utils/errorhandler.h>

#define ROSTERITEMEXCHANGE_UUID "{281C3ACA-AC60-401a-B592-81DFC071A766}"

#define ROSTEREXCHANGE_ACTION_ADD       "add"
#define ROSTEREXCHANGE_ACTION_DELETE    "delete"
#define ROSTEREXCHANGE_ACTION_MODIFY    "modify"

struct IRosterExchangeItem {
	QString action;
	Jid itemJid;
	QString name;
	QSet<QString> groups;
};

struct IRosterExchangeRequest {
	QString id;
	Jid streamJid;
	Jid contactJid;
	QString message;
	QList<IRosterExchangeItem> items;
};

class IRosterItemExchange
{
public:
	virtual QObject *instance() =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString sendExchangeRequest(const IRosterExchangeRequest &ARequest, bool AIqQuery = true) =0;
protected:
	virtual void exchangeRequestSent(const IRosterExchangeRequest &ARequest) =0;
	virtual void exchangeRequestReceived(const IRosterExchangeRequest &ARequest) =0;
	virtual void exchangeRequestApplied(const IRosterExchangeRequest &ARequest) =0;
	virtual void exchangeRequestApproved(const IRosterExchangeRequest &ARequest) =0;
	virtual void exchangeRequestFailed(const IRosterExchangeRequest &ARequest, const ErrorHandler &AError) =0;
};

Q_DECLARE_INTERFACE(IRosterItemExchange,"Vacuum.Plugin.IRosterItemExchange/1.0")

#endif //IROSTERITEMEXCHANGE_H
