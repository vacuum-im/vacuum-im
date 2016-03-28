#ifndef AVATARS_H
#define AVATARS_H

#include <QDir>
#include <QSet>
#include <QRunnable>
#include <QThreadPool>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/options.h>

class LoadAvatarTask :
	public QRunnable
{
public:
	LoadAvatarTask(QObject *AAvatars, const QString &AFile, quint8 ASize, bool AVCard);
	QByteArray parseFile(QFile *AFile) const;
	virtual void run();
public:
	bool FVCard;
	quint8 FSize;
	QString FFile;
	QObject *FAvatars;
public:
	QString FHash;
	QByteArray FData;
	QImage FGrayImage;
	QImage FColorImage;
};

class Avatars :
	public QObject,
	public IPlugin,
	public IAvatars,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IRosterDataHolder,
	public IRostersLabelHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAvatars IStanzaHandler IRosterDataHolder IRostersLabelHolder IStanzaRequestOwner);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.Avatars");
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
	//IAvatars
	virtual quint8 avatarSize(int AType) const;
	virtual bool hasAvatar(const QString &AHash) const;
	virtual QString avatarFileName(const QString &AHash) const;
	virtual QString avatarHash(const Jid &AContactJid, bool AExact=false) const;
	virtual QString saveAvatarData(const QByteArray &AData) const;
	virtual QByteArray loadAvatarData(const QString &AHash) const;
	virtual bool setAvatar(const Jid &AStreamJid, const QByteArray &AData);
	virtual QString setCustomPictire(const Jid &AContactJid, const QByteArray &AData);
	virtual QImage emptyAvatarImage(quint8 ASize, bool AGray=false) const;
	virtual QImage cachedAvatarImage(const QString &AHash, quint8 ASize, bool AGray=false) const;
	virtual QImage loadAvatarImage(const QString &AHash, quint8 ASize, bool AGray=false) const;
	virtual QImage visibleAvatarImage(const QString &AHash, quint8 ASize, bool AGray=false) const;
signals:
	void avatarChanged(const Jid &AContactJid);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
protected:
	QString getImageFormat(const QByteArray &AData) const;
	QByteArray loadFileData(const QString &AFileName) const;
	bool saveFileData(const QString &AFileName, const QByteArray &AData) const;
	void storeAvatarImages(const QString &AHash, quint8 ASize, const QImage &AColor, const QImage &AGray) const;
protected:
	bool startLoadVCardAvatar(const Jid &AContactJid);
	void startLoadAvatarTask(const Jid &AContactJid, LoadAvatarTask *ATask);
protected:
	void updatePresence(const Jid &AStreamJid);
	void updateDataHolder(const Jid &AContactJid);
	bool updateVCardAvatar(const Jid &AContactJid, const QString &AHash, bool AFromVCard);
	bool updateIqAvatar(const Jid &AContactJid, const QString &AHash);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onVCardReceived(const Jid &AContactJid);
	void onVCardPublished(const Jid &AStreamJid);
	void onLoadAvatarTaskFinished(LoadAvatarTask *ATask);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
protected slots:
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
protected slots:
	void onSetAvatarByAction(bool);
	void onClearAvatarByAction(bool);
	void onIconStorageChanged();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IVCardManager *FVCardManager;
	IRostersModel *FRostersModel;
	IStanzaProcessor *FStanzaProcessor;
	IPresenceManager *FPresenceManager;
	IXmppStreamManager *FXmppStreamManager;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	quint8 FAvatarSize;
	bool FAvatarsVisible;
	quint32 FAvatarLabelId;
private:
	QDir FAvatarsDir;
	QImage FEmptyAvatar;
	QMap<Jid, QString> FStreamAvatars;
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
	QMap<Jid, QString> FCustomPictures;
private:
	QThreadPool FThreadPool;
	QHash<QString, LoadAvatarTask *> FFileTasks;
	QHash<LoadAvatarTask *, QSet<Jid> > FTaskContacts;
	mutable QHash<QString, QMap<quint8, QImage> > FAvatarImages;
	mutable QHash<QString, QMap<quint8, QImage> > FAvatarGrayImages;
};

#endif // AVATARS_H
