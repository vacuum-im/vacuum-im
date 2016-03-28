#ifndef DATASTREAMSMANAGER_H
#define DATASTREAMSMANAGER_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ioptionsmanager.h>

class DataStreamsManger :
	public QObject,
	public IPlugin,
	public IDataStreamsManager,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IDataStreamsManager IStanzaHandler IStanzaRequestOwner IOptionsDialogHolder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.DataStreamsManager");
public:
	DataStreamsManger();
	~DataStreamsManger();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return DATASTREAMSMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IDataStreamsManager
	virtual QList<QString> methods() const;
	virtual IDataStreamMethod *method(const QString &AMethodNS) const;
	virtual void insertMethod(IDataStreamMethod *AMethod);
	virtual void removeMethod(IDataStreamMethod *AMethod);
	// Profiles
	virtual QList<QString> profiles() const;
	virtual IDataStreamProfile *profile(const QString &AProfileNS);
	virtual void insertProfile(IDataStreamProfile *AProfile);
	virtual void removeProfile(IDataStreamProfile *AProfile);
	// Settings
	virtual QList<QUuid> settingsProfiles() const;
	virtual QString settingsProfileName(const QUuid &ASettingsId) const;
	virtual OptionsNode settingsProfileNode(const QUuid &ASettingsId, const QString &AMethodNS) const;
	virtual void insertSettingsProfile(const QUuid &ASettingsId, const QString &AName);
	virtual void removeSettingsProfile(const QUuid &ASettingsId);
	// Streams
	virtual bool initStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, const QString &AProfileNS, const QList<QString> &AMethods, int ATimeout =0);
	virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS);
	virtual void rejectStream(const QString &AStreamId, const XmppStanzaError &AError);
signals:
	void methodInserted(IDataStreamMethod *AMethod);
	void methodRemoved(IDataStreamMethod *AMethod);
	void profileInserted(IDataStreamProfile *AProfile);
	void profileRemoved(IDataStreamProfile *AProfile);
	void settingsProfileInserted(const QUuid &ASettingsId);
	void settingsProfileRemoved(const QUuid &ASettingsId);
	void streamInitStarted(const IDataStream &AStream);
	void streamInitFinished(const IDataStream &AStream, const XmppError &AError);
protected:
	QString streamIdByRequestId(const QString &ARequestId) const;
protected slots:
	void onXmppStreamClosed(IXmppStream *AXmppStream);
private:
	IDataForms *FDataForms;
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IStanzaProcessor *FStanzaProcessor;
	IXmppStreamManager *FXmppStreamManager;
private:
	int FSHIInitStream;
	QMap<QString, IDataStream> FStreams;
	QMap<QString, IDataStreamMethod *> FMethods;
	QMap<QString, IDataStreamProfile *> FProfiles;
};

#endif // DATASTREAMSMANAGER_H
