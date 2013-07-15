#ifndef DATASTREAMSMANAGER_H
#define DATASTREAMSMANAGER_H

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/menuicons.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/xmpperror.h>
#include "datastreamsoptions.h"

struct StreamParams {
	Jid streamJid;
	Jid contactJid;
	QString requestId;
	QString profile;
	IDataForm features;
};

class DataStreamsManger :
	public QObject,
	public IPlugin,
	public IDataStreamsManager,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IDataStreamsManager IStanzaHandler IStanzaRequestOwner IOptionsHolder);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IDataStreamsManager");
#endif
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
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IDataStreamsManager
	virtual QList<QString> methods() const;
	virtual IDataStreamMethod *method(const QString &AMethodNS) const;
	virtual void insertMethod(IDataStreamMethod *AMethod);
	virtual void removeMethod(IDataStreamMethod *AMethod);
	virtual QList<QString> profiles() const;
	virtual IDataStreamProfile *profile(const QString &AProfileNS);
	virtual void insertProfile(IDataStreamProfile *AProfile);
	virtual void removeProfile(IDataStreamProfile *AProfile);
	virtual QList<QUuid> settingsProfiles() const;
	virtual QString settingsProfileName(const QUuid &AProfileId) const;
	virtual OptionsNode settingsProfileNode(const QUuid &AProfileId, const QString &AMethodNS) const;
	virtual void insertSettingsProfile(const QUuid &AProfileId, const QString &AName);
	virtual void removeSettingsProfile(const QUuid &AProfileId);
	virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, const QList<QString> &AMethods, int ATimeout =0);
	virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS);
	virtual bool rejectStream(const QString &AStreamId, const XmppStanzaError &AError);
signals:
	void methodInserted(IDataStreamMethod *AMethod);
	void methodRemoved(IDataStreamMethod *AMethod);
	void profileInserted(IDataStreamProfile *AProfile);
	void profileRemoved(IDataStreamProfile *AProfile);
	void settingsProfileInserted(const QUuid &AProfileId, const QString &AName);
	void settingsProfileRemoved(const QUuid &AProfileId);
protected:
	virtual QString streamIdByRequestId(const QString &ARequestId) const;
protected slots:
	void onXmppStreamClosed(IXmppStream *AXmppStream);
private:
	IDataForms *FDataForms;
	IXmppStreams *FXmppStreams;
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	IOptionsManager *FOptionsManager;
private:
	int FSHIInitStream;
private:
	QMap<QString, StreamParams> FStreams;
	QMap<QString, IDataStreamMethod *> FMethods;
	QMap<QString, IDataStreamProfile *> FProfiles;
};

#endif // DATASTREAMSMANAGER_H
