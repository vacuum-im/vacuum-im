#ifndef JABBERSEARCH_H
#define JABBERSEARCH_H

#include <definitions/namespaces.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/dataformtypes.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ijabbersearch.h>
#include <interfaces/idataforms.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresence.h>
#include <utils/xmpperror.h>
#include "searchdialog.h"

class JabberSearch :
			public QObject,
			public IPlugin,
			public IJabberSearch,
			public IStanzaRequestOwner,
			public IDiscoFeatureHandler,
			public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IJabberSearch IStanzaRequestOwner IDiscoFeatureHandler IDataLocalizer);
public:
	JabberSearch();
	~JabberSearch();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return JABBERSEARCH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//IJabberSearch
	virtual QString sendRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendSubmit(const Jid &AStreamJid, const ISearchSubmit &ASubmit);
	virtual void showSearchDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
	void searchFields(const QString &AId, const ISearchFields &AFields);
	void searchResult(const QString &AId, const ISearchResult &AResult);
	void searchError(const QString &AId, const QString &AError);
protected:
	void registerDiscoFeatures();
protected slots:
	void onSearchActionTriggered(bool);
private:
	IPluginManager *FPluginManager;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IPresencePlugin *FPresencePlugin;
	IDataForms *FDataForms;
private:
	QList<QString> FRequests;
	QList<QString> FSubmits;
};

#endif // JABBERSEARCH_H
