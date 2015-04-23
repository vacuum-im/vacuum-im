#ifndef VCARDMANAGER_H
#define VCARDMANAGER_H

#include <QDir>
#include <QTimer>
#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersearch.h>
#include <interfaces/ioptionsmanager.h>
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

class VCardManager :
	public QObject,
	public IPlugin,
	public IVCardManager,
	public IOptionsDialogHolder,
	public IStanzaRequestOwner,
	public IXmppUriHandler,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IVCardManager IRosterDataHolder IStanzaRequestOwner IXmppUriHandler IOptionsDialogHolder);
	friend class VCard;
public:
	VCardManager();
	~VCardManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return VCARD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin()  { return true; }
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IVCardManager
	virtual QString vcardFileName(const Jid &AContactJid) const;
	virtual bool hasVCard(const Jid &AContactJid) const;
	virtual IVCard *getVCard(const Jid &AContactJid);
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool publishVCard(const Jid &AStreamJid, IVCard *AVCard);
	virtual QDialog *showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent = NULL);
signals:
	void vcardReceived(const Jid &AContactJid);
	void vcardPublished(const Jid &AContactJid);
	void vcardError(const Jid &AContactJid, const XmppError &AError);
	// IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void registerDiscoFeatures();
	void unlockVCard(const Jid &AContactJid);
	void restrictVCardImagesSize(IVCard *AVCard);
	void saveVCardFile(const Jid &AContactJid, const QDomElement &AElem) const;
	void removeEmptyChildElements(QDomElement &AElem) const;
	void insertMessageToolBarAction(IMessageToolBarWidget *AWidget);
	QList<Action *> createClipboardActions(const QSet<QString> &AStrings, QObject *AParent) const;
protected slots:
	void onCopyToClipboardActionTriggered(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onShowVCardDialogByAction(bool);
	void onShowVCardDialogByMessageWindowAction(bool);
	void onVCardDialogDestroyed(QObject *ADialog);
	void onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive);
	void onMessageNormalWindowCreated(IMessageNormalWindow *AWindow);
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
protected slots:
	void onUpdateTimerTimeout();
	void onRosterOpened(IRoster *ARoster);
	void onRosterClosed(IRoster *ARoster);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
private:
	IPluginManager *FPluginManager;
	IXmppStreamManager *FXmppStreamManager;
	IRosterManager *FRosterManager;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatManager *FMultiChatManager;
	IServiceDiscovery *FDiscovery;
	IXmppUriQueries *FXmppUriQueries;
	IMessageWidgets *FMessageWidgets;
	IRosterSearch *FRosterSearch;
	IOptionsManager *FOptionsManager;
private:
	QDir FVCardFilesDir;
	QTimer FUpdateTimer;
	QMap<Jid,VCardItem> FVCards;
	QMultiMap<Jid,Jid> FUpdateQueue;
	QMap<QString,Jid> FVCardRequestId;
	QMap<QString,Jid> FVCardPublishId;
	QMap<QString,Stanza> FVCardPublishStanza;
	QMap<Jid,VCardDialog *> FVCardDialogs;
	mutable QHash<Jid,QStringList> FSearchStrings;
};

#endif // VCARDMANAGER_H
