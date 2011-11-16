#ifndef VCARDPLUGIN_H
#define VCARDPLUGIN_H

#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/toolbargroups.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ivcard.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <utils/widgetmanager.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include <utils/shortcuts.h>
#include "vcard.h"
#include "vcarddialog.h"

struct VCardItem
{
	VCardItem() {
		vcard = NULL;
		locks = 0;
	}
	VCard *vcard;
	int locks;
};

class VCardPlugin :
			public QObject,
			public IPlugin,
			public IVCardPlugin,
			public IStanzaRequestOwner,
			public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IVCardPlugin IStanzaRequestOwner IXmppUriHandler);
	friend class VCard;
public:
	VCardPlugin();
	~VCardPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return VCARD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin()  { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IVCardPlugin
	virtual QString vcardFileName(const Jid &AContactJid) const;
	virtual bool hasVCard(const Jid &AContactJid) const;
	virtual IVCard *vcard(const Jid &AContactJid);
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool publishVCard(IVCard *AVCard, const Jid &AStreamJid);
	virtual void showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void vcardReceived(const Jid &AContactJid);
	void vcardPublished(const Jid &AContactJid);
	void vcardError(const Jid &AContactJid, const QString &AError);
protected:
	void unlockVCard(const Jid &AContactJid);
	void saveVCardFile(const QDomElement &AElem, const Jid &AContactJid) const;
	void removeEmptyChildElements(QDomElement &AElem) const;
	void registerDiscoFeatures();
protected slots:
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onShowVCardDialogByAction(bool);
	void onShowVCardDialogByChatWindowAction(bool);
	void onVCardDialogDestroyed(QObject *ADialog);
	void onXmppStreamRemoved(IXmppStream *AXmppStream);
	void onChatWindowCreated(IChatWindow *AWindow);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	IServiceDiscovery *FDiscovery;
	IXmppUriQueries *FXmppUriQueries;
	IMessageWidgets *FMessageWidgets;
private:
	QMap<Jid, VCardItem> FVCards;
	QMap<QString, Jid> FVCardRequestId;
	QMap<QString, Jid> FVCardPublishId;
	QMap<QString, Stanza> FVCardPublishStanza;
	QMap<Jid, VCardDialog *> FVCardDialogs;
};

#endif // VCARDPLUGIN_H
