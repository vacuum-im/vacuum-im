#ifndef IPEP_H
#define IPEP_H

#include <utils/stanza.h>
#include <utils/jid.h>
#include <interfaces/iservicediscovery.h>

#define PEP_UUID                       "{36dbd5c1-a3cd-11df-87fc-001cbf2edcfc}"

class IPEPHandler
{
public:
	virtual QObject *instance() =0;
	virtual bool processPEPEvent(const Stanza &AStanza) =0;
	virtual bool fillDiscoFeature(const QString &ANode, IDiscoFeature &AFeature) =0;
};

class IPEPManager
{
public:
	virtual bool publishItem(const QString &ANode, const QDomElement &AItem) =0;
	virtual bool publishItem(const QString &ANode, const QDomElement &AItem, const Jid& streamJid) =0;
	virtual int insertNodeHandler(const QString &ANode, IPEPHandler *AHandle) =0;
	virtual bool removeNodeHandler(int AHandleId) =0;
	virtual IPEPHandler* nodeHandler(int AHandleId) =0;
};

Q_DECLARE_INTERFACE(IPEPManager,"Vacuum.Plugin.IPEPManager/1.0")
Q_DECLARE_INTERFACE(IPEPHandler,"Vacuum.Plugin.IPEPHandler/1.0")

#endif // IPEP_H
