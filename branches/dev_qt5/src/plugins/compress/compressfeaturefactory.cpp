#include "compressfeaturefactory.h"

#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturefactoryorders.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/logger.h>

CompressFeatureFactory::CompressFeatureFactory()
{
	FXmppStreamManager = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
}

CompressFeatureFactory::~CompressFeatureFactory()
{

}

void CompressFeatureFactory::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Stream Compression");
	APluginInfo->description = tr("Allows to compress a stream of messages sent and received from the server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool CompressFeatureFactory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
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

	return FXmppStreamManager!=NULL;
}

bool CompressFeatureFactory::initObjects()
{
	XmppError::registerError(NS_FEATURE_COMPRESS,XERR_COMPRESS_UNSUPPORTED_METHOD,tr("Unsupported compression method"));
	XmppError::registerError(NS_FEATURE_COMPRESS,XERR_COMPRESS_SETUP_FAILED,tr("Compression setup failed"));

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_UNKNOWN_ERROR,tr("ZLib error"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_OUT_OF_MEMORY,tr("Out of memory for ZLib"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_VERSION_MISMATCH,tr("ZLib version mismatch"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_INVALID_DEFLATE_DATA,tr("Invalid or incomplete deflate data"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_COMPRESS_INVALID_COMPRESSION_LEVEL,tr("Invalid compression level"));

	if (FXmppStreamManager)
	{
		FXmppStreamManager->registerXmppFeature(XFO_COMPRESS,NS_FEATURE_COMPRESS);
		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_COMPRESS,this);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

bool CompressFeatureFactory::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_STREAMCOMPRESS,false);
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> CompressFeatureFactory::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
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

QList<QString> CompressFeatureFactory::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_COMPRESS;
}

IXmppFeature *CompressFeatureFactory::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_COMPRESS)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountByStream(AXmppStream->streamJid()) : NULL;
		if (account==NULL || account->optionsNode().value("stream-compress").toBool())
		{
			LOG_STRM_INFO(AXmppStream->streamJid(),"Compression XMPP stream feature created");
			IXmppFeature *feature = new CompressFeature(AXmppStream);
			connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
			emit featureCreated(feature);
			return feature;
		}
	}
	return NULL;
}

void CompressFeatureFactory::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"Compression XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}
