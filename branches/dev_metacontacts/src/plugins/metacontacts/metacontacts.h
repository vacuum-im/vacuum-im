#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QMap>
#include <QHash>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagewidgets.h>
#include "combinecontactsdialog.h"
#include "metasortfilterproxymodel.h"

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts,
	public IRostersLabelHolder,
	public IRostersClickHooker,
	public IRostersDragDropHandler,
	public IRostersEditHandler,
	public AdvancedDelegateEditProxy
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts IRostersLabelHolder IRostersClickHooker IRostersDragDropHandler IRostersEditHandler);
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject *instance() { return this; }
	// IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	// IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AHover, Menu *AMenu);
	//IRostersEditHandler
	virtual quint32 rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const;
	virtual AdvancedDelegateEditProxy *rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex);
	//AdvancedDelegateEditProxy
	virtual bool setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex);
	// IMetaContacts
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const Jid &AItem) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual QList<IRosterIndex *> findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual QUuid createMetaContact(const Jid &AStreamJid, const QList<Jid> &AItems, const QString &AName);
	virtual bool mergeMetaContacts(const Jid &AStreamJid, const QUuid &AMetaId1, const QUuid &AMetaId2);
	virtual bool detachMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
	virtual bool setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName);
	virtual bool setMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
	virtual bool setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups);
signals:
	void metaContactsOpened(const Jid &AStreamJid);
	void metaContactsClosed(const Jid &AStreamJid);
	void metaContactChanged(const Jid &AStreamJid, const IMetaContact &AMetaContact, const IMetaContact &ABefore);
	// IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
protected:
	bool isValidItem(const Jid &AStreamJid, const Jid &AItem) const;
	void updateMetaIndexes(const Jid &AStreamJid, const IMetaContact &AMetaContact);
	void removeMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId);
	void updateMetaIndexItems(IRosterIndex *AMetaIndex, const QList<Jid> &AItems);
	void updateMetaWindows(const Jid &AStreamJid, const IMetaContact &AMetaContact, const QSet<Jid> ANewItems, const QSet<Jid> AOldItems);
	bool updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact);
	void updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts);
	void startUpdateMetaContact(const Jid &AStreamJid, const QUuid &AMetaId);
	void startUpdateMetaContactIndex(const Jid &AStreamJid, const QUuid &AMetaId, IRosterIndex *AIndex);
protected:
	bool isReadyStreams(const QStringList &AStreams) const;
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	bool isDragDropAccetped(const QDropEvent *AEvent, IRosterIndex *AHover) const;
	QList<IRosterIndex *> indexesProxies(const QList<IRosterIndex *> &AIndexes, bool ASelfProxy=true) const;
protected:
	void renameMetaContact(const Jid &AStreamJid, const QUuid &AMetaId);
	void detachMetaItems(const QStringList &AStreams, const QStringList &AContacts);
	void combineMetaItems(const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas);
	void destroyMetaContacts(const QStringList &AStreams, const QStringList &AMetas);
protected:
	void startSaveContactsToStorage(const Jid &AStreamJid);
	bool saveContactsToStorage(const Jid &AStreamJid) const;
	QString metaContactsFileName(const Jid &AStreamJid) const;
	QList<IMetaContact> loadMetaContactsFromXML(const QDomElement &AElement) const;
	void saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const;
	QList<IMetaContact> loadMetaContactsFromFile(const QString &AFileName) const;
	void saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const;
protected slots:
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
protected slots:
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
	void onPrivateStorageClosed(const Jid &AStreamJid);
protected slots:
	void onRostersModelStreamsLayoutChanged(int ABefore);
	void onRostersModelIndexInserted(IRosterIndex *AIndex);
	void onRostersModelIndexDestroyed(IRosterIndex *AIndex);
protected slots:
	void onRostersViewIndexContextMenuAboutToShow();
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onRostersViewNotifyInserted(int ANotifyId);
	void onRostersViewNotifyRemoved(int ANotifyId);
	void onRostersViewNotifyActivated(int ANotifyId);
protected slots:
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
	void onMessageChatWindowDestroyed();
protected slots:
	void onDetachMetaItemsByAction();
	void onCombineMetaItemsByAction();
	void onRenameMetaContactByAction();
	void onDestroyMetaContactsByAction();
	void onCopyMetaContactToGroupByAction();
	void onMoveMetaContactToGroupByAction();
protected slots:
	void onUpdateContactsTimerTimeout();
	void onLoadContactsFromFileTimerTimeout();
	void onSaveContactsToStorageTimerTimeout();
protected slots:
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStatusIcons *FStatusIcons;
	IMessageWidgets *FMessageWidgets;
private:
	QTimer FSaveTimer;
	QTimer FUpdateTimer;
	QSet<Jid> FSaveStreams;
	QSet<Jid> FLoadStreams;
	QMap<Jid, QString> FLoadRequestId;
	QMap<Jid, QMap<QUuid, QSet<IRosterIndex *> > > FUpdateContacts;
private:
	QMap<Jid, QHash<Jid, QUuid> > FItemMetaId;
	QMap<Jid, QHash<QUuid, IMetaContact> > FMetaContacts;
private:
	QMap<int, int> FProxyToIndexNotify;
	QMap<Menu *, Menu *> FProxyContextMenu;
	MetaSortFilterProxyModel *FSortFilterProxyModel;
	QMap<Jid, QHash<QUuid, QList<IRosterIndex *> > > FMetaIndexes;
	QHash<const IRosterIndex *, IRosterIndex *> FMetaIndexItemProxy;
	QMultiHash<const IRosterIndex *, IRosterIndex *> FMetaIndexProxyItem;
	QHash<const IRosterIndex *, QMap<Jid, IRosterIndex *> > FMetaIndexItems;
private:
	QMap<Jid, QHash<QUuid, IMessageChatWindow *> > FMetaChatWindows;
};

#endif // METACONTACTS_H
