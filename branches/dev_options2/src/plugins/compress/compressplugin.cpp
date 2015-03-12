#include "compressplugin.h"

#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/logger.h>

CompressPlugin::CompressPlugin()
{
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
}

CompressPlugin::~CompressPlugin()
{

}

void CompressPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Stream Compression");
	APluginInfo->description = tr("Allows to compress a stream of messages sent and received from the server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool CompressPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	return FXmppStreams!=NULL;
}

bool CompressPlugin::initObjects()
{
	XmppError::registerError(NS_FEATURE_COMPRESS,XERR_COMPRESS_UNSUPPORTED_METHOD,tr("Unsupported compression method"));
	XmppError::registerError(NS_FEATURE_COMPRESS,XERR_COMPRESS_SETUP_FAILED,tr("Compression setup failed"));

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_UNKNOWN_ERROR,tr("ZLib error"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_OUT_OF_MEMORY,tr("Out of memory for ZLib"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_VERSION_MISMATCH,tr("ZLib version mismatch"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_INVALID_DEFLATE_DATA,tr("Invalid or incomplete deflate data"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_INVALID_COMPRESSION_LEVEL,tr("Invalid compression level"));

	if (FXmppStreams)
	{
		FXmppStreams->registerXmppFeature(XFO_COMPRESS,NS_FEATURE_COMPRESS);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_COMPRESS,this);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

bool CompressPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_STREAMCOMPRESS,false);
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> CompressPlugin::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager)
	{
		QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
		if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="Additional")
		{
			OptionsNode options = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));
			widgets.insertMulti(OWO_ACCOUNTS_ADDITIONAL_STREAMCOMPRESS, FOptionsManager->newOptionsDialogWidget(options.node("stream-compress"),tr("Enable data compression transferred between client and server"),AParent));
		}
	}
	return widgets;
}

QList<QString> CompressPlugin::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_COMPRESS;
}

IXmppFeature *CompressPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_COMPRESS)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountByStream(AXmppStream->streamJid()) : NULL;
		if (account==NULL || account->optionsNode().value("stream-compress").toBool())
		{
			LOG_STRM_INFO(AXmppStream->streamJid(),"Compression XMPP stream feature created");
			IXmppFeature *feature = new Compression(AXmppStream);
			connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
			emit featureCreated(feature);
			return feature;
		}
	}
	return NULL;
}

void CompressPlugin::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"Compression XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}

Q_EXPORT_PLUGIN2(plg_compress, CompressPlugin)
