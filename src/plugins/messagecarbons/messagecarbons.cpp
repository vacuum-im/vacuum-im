#include "messagecarbons.h"

#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/message.h>
#include <utils/stanza.h>
#include <utils/logger.h>

#define CARBONS_TIMEOUT               30000

#define SHC_CARBONS_SENT              "/message/sent[@xmlns='" NS_MESSAGE_CARBONS "']"
#define SHC_CARBONS_RECEIVED          "/message/received[@xmlns='" NS_MESSAGE_CARBONS "']"

MessageCarbons::MessageCarbons()
{
	FXmppStreamManager = NULL;
	FDiscovery = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
}

MessageCarbons::~MessageCarbons()
{

}

void MessageCarbons::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Carbons");
	APluginInfo->description = tr("Allows to keep all user IM clients engaged in a conversation");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
}

bool MessageCarbons::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL && FStanzaProcessor!=NULL && FDiscovery!=NULL;
}

bool MessageCarbons::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature dfeature;
		dfeature.var = NS_MESSAGE_CARBONS;
		dfeature.active = true;
		dfeature.name = tr("Message Carbons");
		dfeature.description = tr("Allows to keep all user IM clients engaged in a conversation");
		FDiscovery->insertDiscoFeature(dfeature);
	}
	return true;
}

bool MessageCarbons::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (isEnabled(AStreamJid) && FSHIForwards.value(AStreamJid)==AHandleId)
	{
		QDomElement sentElem = AStanza.firstElement("sent",NS_MESSAGE_CARBONS);
		QDomElement recvElem = AStanza.firstElement("received",NS_MESSAGE_CARBONS);
		QDomElement carbonElem = !sentElem.isNull() ? sentElem : recvElem;
		QDomElement messageElem =  AStanza.findElement(carbonElem,"forwarded",NS_MESSAGE_FORWARD).firstChildElement("message");
		if (!messageElem.isNull())
		{
			AAccept = true;
			Stanza stanza(messageElem);
			Message message(stanza);
			if (!sentElem.isNull())
			{
				message.stanza().addElement("sent",NS_MESSAGE_CARBONS);
				if (FMessageProcessor)
				{
					if (FMessageProcessor->processMessage(AStreamJid,message,IMessageProcessor::DirectionOut))
						FMessageProcessor->displayMessage(AStreamJid,message,IMessageProcessor::DirectionOut);
				}
				emit messageSent(AStreamJid,message);
			}
			else if (!recvElem.isNull())
			{
				message.stanza().addElement("received",NS_MESSAGE_CARBONS);
				if (FMessageProcessor)
				{
					if (FMessageProcessor->processMessage(AStreamJid,message,IMessageProcessor::DirectionIn))
						FMessageProcessor->displayMessage(AStreamJid,message,IMessageProcessor::DirectionIn);
				}
				emit messageReceived(AStreamJid,message);
			}
		}
	}
	return false;
}

void MessageCarbons::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FEnableRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Message carbons enabled, id=%1").arg(AStanza.id()));
			FEnabled[AStreamJid] = true;
			emit enableChanged(AStreamJid,true);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to enable message carbons, id=%1: %2").arg(AStanza.id(),err.condition()));
			emit errorReceived(AStreamJid,err);
		}
		FEnableRequests.removeAll(AStanza.id());
	}
	else if(FDisableRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Message carbons disabled, id=%1").arg(AStanza.id()));
			FEnabled[AStreamJid] = false;
			emit enableChanged(AStreamJid,false);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to disable message carbons, id=%1: %2").arg(AStanza.id(),err.condition()));
			emit errorReceived(AStreamJid,err);
		}
		FDisableRequests.removeAll(AStanza.id());
	}
}

bool MessageCarbons::isSupported(const Jid &AStreamJid) const
{
	return FDiscovery!=NULL && FDiscovery->discoInfo(AStreamJid,AStreamJid.domain()).features.contains(NS_MESSAGE_CARBONS);
}

bool MessageCarbons::isEnabled(const Jid &AStreamJid) const
{
	return FEnabled.value(AStreamJid);
}

bool MessageCarbons::setEnabled(const Jid &AStreamJid, bool AEnable)
{
	if (FStanzaProcessor && isSupported(AStreamJid))
	{
		if (AEnable != isEnabled(AStreamJid))
		{
			Stanza request("iq");
			request.setType("set").setId(FStanzaProcessor->newId());
			request.addElement(AEnable ? "enable" : "disable",NS_MESSAGE_CARBONS);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,CARBONS_TIMEOUT))
			{
				if (AEnable)
				{
					LOG_STRM_INFO(AStreamJid,QString("Enable message carbons request sent, id=%1").arg(request.id()));
					FEnableRequests.append(request.id());
				}
				else
				{
					LOG_STRM_INFO(AStreamJid,QString("Disable message carbons request sent, id=%1").arg(request.id()));
					FDisableRequests.append(request.id());
				}
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,"Failed to send enable/disable message carbons request");
			}
			return false;
		}
		return true;
	}
	return false;
}

void MessageCarbons::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_CARBONS_SENT);
		shandle.conditions.append(SHC_CARBONS_RECEIVED);
		FSHIForwards.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
}

void MessageCarbons::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHIForwards.take(AXmppStream->streamJid()));
	FEnabled.remove(AXmppStream->streamJid());
}

void MessageCarbons::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.contactJid==AInfo.streamJid.domain() && !FEnabled.contains(AInfo.streamJid))
	{
		FEnabled.insert(AInfo.streamJid,false);
		if (AInfo.features.contains(NS_MESSAGE_CARBONS))
			setEnabled(AInfo.streamJid,true);
	}
}
