#ifndef AVATARS_H
#define AVATARS_H

#include <QDir>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/vcardvaluenames.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ivcard.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/imagemanager.h>

class Avatars :
	public QObject,
	public IPlugin,
	public IAvatars,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IRosterDataHolder,
	public IRostersLabelHolder,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAvatars IStanzaHandler IRosterDataHolder IRostersLabelHolder IStanzaRequestOwner IOptionsHolder);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IAvatars");
#endif
public:
	Avatars();
	~Avatars();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return AVATARTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IAvatars
	virtual QString avatarHash(const Jid &AContactJid) const;
	virtual bool hasAvatar(const QString &AHash) const;
	virtual QString avatarFileName(const QString &AHash) const;
	virtual QString saveAvatarData(const QByteArray &AData) const;
	virtual QByteArray loadAvatarData(const QString &AHash) const;
	virtual bool setAvatar(const Jid &AStreamJid, const QByteArray &AData);
	virtual QString setCustomPictire(const Jid &AContactJid, const QByteArray &AData);
	virtual QImage emptyAvatarImage(const QSize &AMaxSize = QSize(), bool AGray = false) const;
	virtual QImage loadAvatarImage(const QString &AHash, const QSize &AMaxSize = QSize(), bool AGray = false) const;
signals:
	void avatarChanged(const Jid &AContactJid);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
protected:
	QString getImageFormat(const QByteArray &AData) const;
	QByteArray loadFromFile(const QString &AFileName) const;
	bool saveToFile(const QString &AFileName, const QByteArray &AData) const;
	QByteArray loadAvatarFromVCard(const Jid &AContactJid) const;
	void updatePresence(const Jid &AStreamJid) const;
	void updateDataHolder(const Jid &AContactJid = Jid::null);
	bool updateVCardAvatar(const Jid &AContactJid, const QString &AHash, bool AFromVCard);
	bool updateIqAvatar(const Jid &AContactJid, const QString &AHash);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onVCardChanged(const Jid &AContactJid);
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onSetAvatarByAction(bool);
	void onClearAvatarByAction(bool);
	void onIconStorageChanged();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IVCardPlugin *FVCardPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IOptionsManager *FOptionsManager;
private:
	QMap<Jid, int> FSHIPresenceIn;
	QMap<Jid, int> FSHIPresenceOut;
	QHash<Jid, QString> FVCardAvatars;
	QMultiMap<Jid, Jid> FBlockingResources;
private:
	QMap<Jid, int> FSHIIqAvatarIn;
	QHash<Jid, QString> FIqAvatars;
	QMap<QString, Jid> FIqAvatarRequests;
private:
	QSize FAvatarSize;
	bool FAvatarsVisible;
	bool FShowEmptyAvatars;
	bool FShowGrayAvatars;
	QMap<Jid, QString> FCustomPictures;
private:
	quint32 FAvatarLabelId;
	QDir FAvatarsDir;
	QImage FEmptyAvatar;
	QImage FGrayEmptyAvatar;
	QMap<Jid, QString> FStreamAvatars;
	mutable QHash<QString, QMap<QSize,QImage> > FAvatarImages;
	mutable QHash<QString, QMap<QSize,QImage> > FGrayAvatarImages;
};

#endif // AVATARS_H
