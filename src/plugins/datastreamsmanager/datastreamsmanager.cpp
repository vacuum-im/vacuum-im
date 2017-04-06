#include "datastreamsmanager.h"

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/menuicons.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/stanza.h>
#include <utils/logger.h>
#include <utils/jid.h>

#define DFV_STREAM_METHOD             "stream-method"

#define SHC_INIT_STREAM               "/iq[@type='set']/si[@xmlns='" NS_STREAM_INITIATION "']"

DataStreamsManger::DataStreamsManger()
{
	FDataForms = NULL;
	FDiscovery = NULL;
	FXmppStreamManager = NULL;
	FStanzaProcessor = NULL;
	FOptionsManager = NULL;

	FSHIInitStream = -1;
}

DataStreamsManger::~DataStreamsManger()
{

}

void DataStreamsManger::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Data Streams Manager");
	APluginInfo->description = tr("Allows to initiate a custom stream of data between two XMPP entities");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool DataStreamsManger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	return FDataForms!=NULL && FStanzaProcessor!=NULL;
}


bool DataStreamsManger::initObjects()
{
	XmppError::registerError(NS_STREAM_INITIATION,XERR_SI_BAD_PROFILE,tr("The profile is not understood or invalid"));
	XmppError::registerError(NS_STREAM_INITIATION,XERR_SI_NO_VALID_STREAMS,tr("None of the available streams are acceptable"));

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_STREAMID_EXISTS,tr("Stream with same ID already exists"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_INVALID_RESPONSE,tr("Invalid stream initiation response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_UNSUPPORTED_REQUEST,tr("Unsupported stream initiation request"));

	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_INIT_STREAM);
		FSHIInitStream = FStanzaProcessor->insertStanzaHandle(shandle);
	}

	if (FDiscovery)
	{
		IDiscoFeature si;
		si.active = true;
		si.var = NS_STREAM_INITIATION;
		si.name = tr("Data Streams Initiation");
		si.description = tr("Supports the initiating of the custom stream of data between two XMPP entities");
		FDiscovery->insertDiscoFeature(si);
	}

	return true;
}

bool DataStreamsManger::initSettings()
{
	Options::setDefaultValue(OPV_DATASTREAMS_SPROFILE_NAME,tr("<Default Profile>"));

	if (FOptionsManager)
	{
		IOptionsDialogNode dataNode = { ONO_DATATRANSFER, OPN_DATATRANSFER, MNI_DATASTREAMSMANAGER, tr("Data Transfer") };
		FOptionsManager->insertOptionsDialogNode(dataNode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> DataStreamsManger::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_DATATRANSFER)
	{
		int index = 0;
		foreach(IDataStreamMethod *method, FMethods)
		{
			widgets.insertMulti(OHO_DATATRANSFER_METHODNAME + index, FOptionsManager->newOptionsDialogHeader(tr("Transfer method %1").arg(method->methodName()),AParent));
			widgets.insertMulti(OWO_DATATRANSFER_METHODSETTINGS + index, method->methodSettingsWidget(settingsProfileNode(QUuid(),method->methodNS()),AParent));
			index += 10;
		}
	}
	return widgets;
}

bool DataStreamsManger::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FDataForms && AHandlerId==FSHIInitStream)
	{
		AAccept = true;

		XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
		QDomElement siElem = AStanza.firstElement("si",NS_STREAM_INITIATION);
		
		IDataStream stream;
		stream.kind = IDataStream::Target;
		stream.streamJid = AStreamJid;
		stream.contactJid = AStanza.from();
		stream.streamId = siElem.attribute("id");
		stream.requestId = AStanza.id();
		stream.profileNS = siElem.attribute("profile");

		IDataStreamProfile *sprofile = FProfiles.value(stream.profileNS);
		if (sprofile != NULL)
		{
			QDomElement fnegElem = siElem.firstChildElement("feature");
			while (!fnegElem.isNull() && fnegElem.namespaceURI()!=NS_FEATURENEG)
				fnegElem = fnegElem.nextSiblingElement("feature");

			QDomElement formElem = fnegElem.firstChildElement("x");
			while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
				formElem = formElem.nextSiblingElement("x");
			stream.features = FDataForms->dataForm(formElem);

			QList<QString> methods;
			int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,stream.features.fields);
			if (index >= 0)
			{
				foreach(const IDataOption &option, stream.features.fields.at(index).options)
					if (FMethods.contains(option.value))
						methods.append(option.value);
			}
			
			if (!stream.streamId.isEmpty() && !methods.isEmpty() && !FStreams.contains(stream.streamId))
			{
				FStreams.insert(stream.streamId,stream);
				LOG_STRM_INFO(AStreamJid,QString("Data stream initiation request received from=%1, profile=%2, sid=%3").arg(AStanza.from(),stream.profileNS,stream.streamId));

				if (!sprofile->dataStreamProcessRequest(stream.streamId,AStanza,methods))
				{
					FStreams.remove(stream.streamId);
					LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation request from=%1, profile=%2: Unsupported request").arg(AStanza.from(),stream.profileNS));
					err.setAppCondition(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_UNSUPPORTED_REQUEST);
				}
				else
				{
					emit streamInitStarted(stream);
					return true;
				}
			}
			else if (stream.streamId.isEmpty())
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation request from=%1: Invalid stream id").arg(AStanza.from()));
			}
			else if (methods.isEmpty())
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation request from=%1: No valid stream methods").arg(AStanza.from()));
				err.setAppCondition(NS_STREAM_INITIATION,XERR_SI_NO_VALID_STREAMS);
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation request from=%1: Duplicate stream id=%2").arg(AStanza.from(),stream.streamId));
				err.setAppCondition(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_STREAMID_EXISTS);
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation request from=%1: Bad profile=%2").arg(AStanza.from(),stream.profileNS));
			err.setAppCondition(NS_STREAM_INITIATION,XERR_SI_BAD_PROFILE);
		}

		Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
		FStanzaProcessor->sendStanzaOut(AStreamJid,error);

		emit streamInitFinished(stream,err);
	}
	return false;
}

void DataStreamsManger::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	QString streamId = streamIdByRequestId(AStanza.id());
	if (FDataForms && FStreams.contains(streamId))
	{
		IDataStream stream = FStreams.value(streamId);
		IDataStreamProfile *sprofile = FProfiles.value(stream.profileNS);
		if (sprofile != NULL)
		{
			if (AStanza.isResult())
			{
				QDomElement fnegElem = AStanza.firstElement("si",NS_STREAM_INITIATION).firstChildElement("feature");
				while (!fnegElem.isNull() && fnegElem.namespaceURI()!=NS_FEATURENEG)
					fnegElem = fnegElem.nextSiblingElement("feature");

				QDomElement formElem = fnegElem.firstChildElement("x");
				while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
					formElem = formElem.nextSiblingElement("x");
				IDataForm form = FDataForms->dataForm(formElem);

				int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,form.fields);
				QString method = index>=0 ? form.fields.at(index).value.toString() : QString::null;

				if (FMethods.contains(method) && FDataForms->isSubmitValid(stream.features,form))
				{
					LOG_STRM_INFO(AStreamJid,QString("Data stream initiation response received from=%1, sid=%2").arg(AStanza.from(),streamId));
					if (sprofile->dataStreamProcessResponse(streamId,AStanza,method))
					{
						FStreams.remove(streamId);
						emit streamInitFinished(stream,XmppError::null);
					}
				}
				else
				{
					XmppError err(IERR_DATASTREAMS_STREAM_INVALID_RESPONSE);
					LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation response from=%1, sid=%2: %3").arg(AStanza.from(),streamId,err.condition()));
					sprofile->dataStreamProcessError(streamId,err);
				}
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_WARNING(AStreamJid,QString("Failed to process data stream initiation response from=%1, sid=%2: %3").arg(AStanza.from(),streamId,err.condition()));
				sprofile->dataStreamProcessError(streamId,err);
			}
		}
		else
		{
			REPORT_ERROR("Failed to process data stream response: Profile not found");
		}

		if (FStreams.contains(streamId))
		{
			FStreams.remove(streamId);
			emit streamInitFinished(stream,XmppStanzaError(XmppStanzaError::EC_INTERNAL_SERVER_ERROR));
		}
	}
}

QList<QString> DataStreamsManger::methods() const
{
	return FMethods.keys();
}

IDataStreamMethod *DataStreamsManger::method(const QString &AMethodNS) const
{
	return FMethods.value(AMethodNS);
}

void DataStreamsManger::insertMethod(IDataStreamMethod *AMethod)
{
	if (AMethod!=NULL && !FMethods.contains(AMethod->methodNS()) && !FMethods.values().contains(AMethod))
	{
		LOG_DEBUG(QString("Stream method inserted, ns=%1").arg(AMethod->methodNS()));
		FMethods.insert(AMethod->methodNS(),AMethod);
		emit methodInserted(AMethod);
	}
}

void DataStreamsManger::removeMethod(IDataStreamMethod *AMethod)
{
	if (FMethods.values().contains(AMethod))
	{
		LOG_DEBUG(QString("Stream method removed, ns=%1").arg(AMethod->methodNS()));
		FMethods.remove(FMethods.key(AMethod));
		emit methodRemoved(AMethod);
	}
}

QList<QString> DataStreamsManger::profiles() const
{
	return FProfiles.keys();
}

IDataStreamProfile *DataStreamsManger::profile(const QString &AProfileNS)
{
	return FProfiles.value(AProfileNS);
}

void DataStreamsManger::insertProfile(IDataStreamProfile *AProfile)
{
	if (AProfile!=NULL && !FProfiles.contains(AProfile->dataStreamProfile()) && !FProfiles.values().contains(AProfile))
	{
		LOG_DEBUG(QString("Stream profile inserted, ns=%1").arg(AProfile->dataStreamProfile()));
		FProfiles.insert(AProfile->dataStreamProfile(),AProfile);
		emit profileInserted(AProfile);
	}
}

void DataStreamsManger::removeProfile(IDataStreamProfile *AProfile)
{
	if (FProfiles.values().contains(AProfile))
	{
		LOG_DEBUG(QString("Stream profile removed, ns=%1").arg(AProfile->dataStreamProfile()));
		FProfiles.remove(FProfiles.key(AProfile));
		emit profileRemoved(AProfile);
	}
}

QList<QUuid> DataStreamsManger::settingsProfiles() const
{
	QList<QUuid> sprofiles;
	sprofiles.append(QUuid().toString());

	foreach(const QString &sprofile, Options::node(OPV_DATASTREAMS_ROOT).childNSpaces("settings-profile"))
		if (!sprofiles.contains(sprofile))
			sprofiles.append(sprofile);

	return sprofiles;
}

QString DataStreamsManger::settingsProfileName(const QUuid &ASettingsId) const
{
	return Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,ASettingsId.toString()).value("name").toString();
}

OptionsNode DataStreamsManger::settingsProfileNode(const QUuid &ASettingsId, const QString &AMethodNS) const
{
	return Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,ASettingsId.toString()).node("method",AMethodNS);
}

void DataStreamsManger::insertSettingsProfile(const QUuid &ASettingsId, const QString &AName)
{
	if (!ASettingsId.isNull() && !AName.isEmpty())
	{
		Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,ASettingsId.toString()).setValue(AName,"name");
		emit settingsProfileInserted(ASettingsId);
	}
}

void DataStreamsManger::removeSettingsProfile(const QUuid &AProfileId)
{
	if (!AProfileId.isNull())
	{
		Options::node(OPV_DATASTREAMS_ROOT).removeChilds("settings-profile",AProfileId.toString());
		emit settingsProfileRemoved(AProfileId.toString());
	}
}

bool DataStreamsManger::initStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, const QString &AProfileNS, const QList<QString> &AMethods, int ATimeout)
{
	if (FStanzaProcessor && FDataForms && !AStreamId.isEmpty() && !FStreams.contains(AStreamId) && !FMethods.isEmpty())
	{
		IDataStreamProfile *sprofile = FProfiles.value(AProfileNS);
		if (sprofile)
		{
			Stanza request(STANZA_KIND_IQ);
			request.setType(STANZA_TYPE_SET).setTo(AContactJid.full()).setUniqueId();

			QDomElement siElem = request.addElement("si",NS_STREAM_INITIATION);
			siElem.setAttribute("id",AStreamId);
			siElem.setAttribute("profile",AProfileNS);

			IDataField field;
			field.var = DFV_STREAM_METHOD;
			field.type = DATAFIELD_TYPE_LISTSINGLE;
			foreach(const QString &smethod, AMethods)
			{
				if (FMethods.contains(smethod))
				{
					IDataOption option;
					option.value = smethod;
					field.options.append(option);
				}
			}

			if (!field.options.isEmpty() && sprofile->dataStreamMakeRequest(AStreamId,request))
			{
				IDataForm form;
				form.type = DATAFORM_TYPE_FORM;
				form.fields.append(field);
				QDomElement negElem = siElem.appendChild(request.createElement("feature",NS_FEATURENEG)).toElement();
				FDataForms->xmlForm(form,negElem);

				if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,ATimeout))
				{
					IDataStream stream;
					stream.kind = IDataStream::Initiator;
					stream.streamJid = AStreamJid;
					stream.contactJid = AContactJid;
					stream.streamId = AStreamId;
					stream.requestId = request.id();
					stream.profileNS = AProfileNS;
					stream.features = form;
					FStreams.insert(AStreamId,stream);

					LOG_STRM_INFO(AStreamJid,QString("Data stream initiation request sent, to=%1, profile=%2, sid=%3").arg(AContactJid.full(),AProfileNS,AStreamId));
					emit streamInitStarted(stream);

					return true;
				}
				else
				{
					LOG_STRM_WARNING(AStreamJid,QString("Failed to send data stream initiation request to=%1, profile=%2, sid=%3: Request not sent").arg(AContactJid.full(),AProfileNS,AStreamId));
				}
			}
			else if (field.options.isEmpty())
			{
				LOG_WARNING(QString("Failed to send data stream initiation request to=%1, profile=%2, sid=%3: Methods not supported").arg(AContactJid.full(),AProfileNS,AStreamId));
			}
			else
			{
				LOG_WARNING(QString("Failed to send data stream initiation request to=%1, profile=%2, sid=%3: Unsupported request").arg(AContactJid.full(),AProfileNS,AStreamId));
			}
		}
		else
		{
			LOG_WARNING(QString("Failed to send data stream initiation request, to=%1, profile=%2, sid=%3: Profile not found").arg(AContactJid.full(),AProfileNS,AStreamId));
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to send data stream initiation request: Invalid params");
	}
	return false;
}

bool DataStreamsManger::acceptStream(const QString &AStreamId, const QString &AMethodNS)
{
	if (FStanzaProcessor && FDataForms && FStreams.contains(AStreamId) && FMethods.contains(AMethodNS))
	{
		IDataStream stream = FStreams.value(AStreamId);
		IDataStreamProfile *sprofile = FProfiles.value(stream.profileNS);
		int methodField = FDataForms->fieldIndex(DFV_STREAM_METHOD,stream.features.fields);
		if (sprofile!=NULL && methodField>=0 && FDataForms->isOptionValid(stream.features.fields.at(methodField).options,AMethodNS))
		{
			Stanza reply(STANZA_KIND_IQ);
			reply.setType(STANZA_TYPE_RESULT).setTo(stream.contactJid.full()).setId(stream.requestId);

			QDomElement siElem = reply.addElement("si",NS_STREAM_INITIATION);
			if (sprofile->dataStreamMakeResponse(AStreamId,reply))
			{
				QDomElement negElem = siElem.appendChild(reply.createElement("feature",NS_FEATURENEG)).toElement();
				stream.features.fields[methodField].value = AMethodNS;
				FDataForms->xmlForm(FDataForms->dataSubmit(stream.features),negElem);

				if (FStanzaProcessor->sendStanzaOut(stream.streamJid,reply))
				{
					FStreams.remove(AStreamId);
					LOG_STRM_INFO(stream.streamJid,QString("Data stream initiation response sent to=%1, sid=%2, method=%3").arg(stream.contactJid.full(),AStreamId,AMethodNS));
					emit streamInitFinished(stream,XmppError::null);
					return true;
				}
				else
				{
					LOG_STRM_WARNING(stream.streamJid,QString("Failed to send data stream initiation response to=%1, sid=%2").arg(stream.contactJid.full(),AStreamId));
				}
			}
			else
			{
				LOG_STRM_WARNING(stream.streamJid,QString("Failed to send data stream initiation response to=%1, sid=%2: Unsupported response").arg(stream.contactJid.full(),AStreamId));
			}
		}
		else if (sprofile == NULL)
		{
			REPORT_ERROR("Failed to send data stream initiation response: Profile not found");
		}
		else
		{
			REPORT_ERROR("Failed to send data stream initiation response: Method not supported");
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to send data stream initiation response: Invalid params");
	}
	return false;
}

void DataStreamsManger::rejectStream(const QString &AStreamId, const XmppStanzaError &AError)
{
	if (FStanzaProcessor && FStreams.contains(AStreamId))
	{
		IDataStream stream = FStreams.take(AStreamId);
		
		Stanza reply(STANZA_KIND_IQ);
		reply.setFrom(stream.contactJid.full()).setId(stream.requestId);
		reply = FStanzaProcessor->makeReplyError(reply,AError);

		if (FStanzaProcessor->sendStanzaOut(stream.streamJid,reply))
			LOG_STRM_INFO(stream.streamJid,QString("Data stream initiation reject sent to=%1, sid=%2: %3").arg(stream.contactJid.full(),AStreamId,AError.condition()));
		else
			LOG_STRM_WARNING(stream.streamJid,QString("Failed to send data stream initiation reject to=%1, sid=%2: Reject not sent").arg(stream.contactJid.full(),AStreamId));

		emit streamInitFinished(stream,AError);
	}
	else if (FStanzaProcessor)
	{
		REPORT_ERROR("Failed to send data stream initiation reject: Stream not found");
	}
}

QString DataStreamsManger::streamIdByRequestId(const QString &ARequestId) const
{
	for (QMap<QString, IDataStream>::const_iterator it = FStreams.constBegin(); it!=FStreams.constEnd(); ++it)
		if (it->requestId == ARequestId)
			return it.key();
	return QString::null;
}

void DataStreamsManger::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	XmppStanzaError err(XmppStanzaError::EC_RECIPIENT_UNAVAILABLE);
	foreach(const IDataStream &stream, FStreams.values())
	{
		if (stream.streamJid == AXmppStream->streamJid())
		{
			IDataStreamProfile *sprofile = FProfiles.value(stream.profileNS);
			if (sprofile != NULL)
				sprofile->dataStreamProcessError(stream.streamId,err);
			
			if (FStreams.contains(stream.streamId))
			{
				FStreams.remove(stream.streamId);
				emit streamInitFinished(stream,err);
			}
		}
	}
}
