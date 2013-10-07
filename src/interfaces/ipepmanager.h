#ifndef IPEPMANAGER_H
#define IPEPMANAGER_H

#include <QDomElement>
#include <utils/jid.h>
#include <utils/stanza.h>

#define PEPMANAGER_UUID "{36dbd5c1-a3cd-11df-87fc-001cbf2edcfc}"

class IPEPHandler
{
public:
	virtual QObject *instance() =0;
	virtual bool processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza) =0;
};

class IPEPManager
{
public:
	virtual bool publishItem(const Jid &AStreamJid, const QString &ANode, const QDomElement &AItem) =0;
	virtual IPEPHandler *nodeHandler(int AHandleId) const =0;
	virtual int insertNodeHandler(const QString &ANode, IPEPHandler *AHandle) =0;
	virtual bool removeNodeHandler(int AHandleId) =0;
};

Q_DECLARE_INTERFACE(IPEPHandler,"Vacuum.Plugin.IPEPHandler/1.0")
Q_DECLARE_INTERFACE(IPEPManager,"Vacuum.Plugin.IPEPManager/1.0")

#endif // IPEPMANAGER_H
