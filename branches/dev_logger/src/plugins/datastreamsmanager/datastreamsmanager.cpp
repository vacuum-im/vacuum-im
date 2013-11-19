#include "datastreamsmanager.h"

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/menuicons.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
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
	FXmppStreams = NULL;
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
		if (FDataForms == NULL)
			LOG_WARNING("Failed to load required interface: IDataForms");
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
		if (FStanzaProcessor == NULL)
			LOG_WARNING("Failed to load required interface: IStanzaProcessor");
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
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

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_PROFILE_INVALID_SETTINGS,tr("Invalid profile settings"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_STREAMID_EXISTS,tr("Stream with same ID already exists"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_INVALID_INIT_RESPONCE,tr("Invalid stream initiation response"));

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
		IDiscoFeature dfeature;
		dfeature.var = NS_STREAM_INITIATION;
		dfeature.active = true;
		dfeature.name = tr("Data Streams Initiation");
		dfeature.description = tr("Supports the initiating of the custom stream of data between two XMPP entities");
		FDiscovery->insertDiscoFeature(dfeature);
	}

	return true;
}

bool DataStreamsManger::initSettings()
{
	Options::setDefaultValue(OPV_DATASTREAMS_SPROFILE_NAME,tr("<Default Profile>"));

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_DATASTREAMS, OPN_DATASTREAMS, tr("Data Streams"), MNI_DATASTREAMSMANAGER };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> DataStreamsManger::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_DATASTREAMS)
	{
		widgets.insertMulti(OWO_DATASTREAMS, new DataStreamsOptions(this,AParent));
	}
	return widgets;
}

bool DataStreamsManger::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FDataForms && AHandlerId==FSHIInitStream)
	{
		AAccept = true;
		QDomElement siElem = AStanza.firstElement("si",NS_STREAM_INITIATION);
		IDataStreamProfile *sprofile = FProfiles.value(siElem.attribute("profile"),NULL);
		if (sprofile)
		{
			QDomElement fnegElem = siElem.firstChildElement("feature");
			while (!fnegElem.isNull() && fnegElem.namespaceURI()!=NS_FEATURENEG)
				fnegElem = fnegElem.nextSiblingElement("feature");

			QDomElement formElem = fnegElem.firstChildElement("x");
			while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
				formElem = formElem.nextSiblingElement("x");
			IDataForm form = FDataForms->dataForm(formElem);

			QList<QString> smethods;
			int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,form.fields);
			if (index >= 0)
			{
				foreach(const IDataOption &option, form.fields.at(index).options)
					if (FMethods.contains(option.value))
						smethods.append(option.value);
			}

			if (!smethods.isEmpty())
			{
				QString sid = siElem.attribute("id");
				if (!FStreams.contains(sid))
				{
					StreamParams params;
					params.streamJid = AStreamJid;
					params.contactJid = AStanza.from();
					params.requestId = AStanza.id();
					params.profile = siElem.attribute("profile");
					params.features = form;
					FStreams.insert(sid,params);
					if (sid.isEmpty() || !sprofile->dataStreamRequest(sid,AStanza,smethods))
					{
						FStreams.remove(sid);
						LOG_STRM_WARNING(AStreamJid,QString("Rejected stream initiation from=%1: Invalid profile=%2 settings").arg(AStanza.from(),params.profile));
						XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
						err.setAppCondition(NS_INTERNAL_ERROR,IERR_DATASTREAMS_PROFILE_INVALID_SETTINGS);
						Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
						FStanzaProcessor->sendStanzaOut(AStreamJid,error);
					}
					else
					{
						LOG_STRM_INFO(AStreamJid,QString("Accepted stream initiation from=%1, profile=%2, sid=%3").arg(AStanza.from(),params.profile,sid));
					}
				}
				else
				{
					LOG_STRM_WARNING(AStreamJid,QString("Rejected stream initiation from=%1: Duplicate stream id=%2").arg(AStanza.from(),sid));
					XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
					err.setAppCondition(NS_INTERNAL_ERROR,IERR_DATASTREAMS_STREAM_STREAMID_EXISTS);
					Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
					FStanzaProcessor->sendStanzaOut(AStreamJid,error);
				}
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Rejected stream initiation from=%1: No valid stream methods").arg(AStanza.from()));
				XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
				err.setAppCondition(NS_STREAM_INITIATION,XERR_SI_NO_VALID_STREAMS);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Rejected stream initiation from=%1: Bad profile=%2").arg(AStanza.from(),siElem.attribute("profile")));
			XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
			err.setAppCondition(NS_STREAM_INITIATION,XERR_SI_BAD_PROFILE);
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
	}
	else if (FDataForms)
	{
		REPORT_ERROR("Received unexpected stanza");
	}
	return false;
}

void DataStreamsManger::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	QString sid = streamIdByRequestId(AStanza.id());
	if (FDataForms && FStreams.contains(sid))
	{
		StreamParams params = FStreams.take(sid);
		IDataStreamProfile *sprofile = FProfiles.value(params.profile);
		if (sprofile)
		{
			if (AStanza.type() == "result")
			{
				QDomElement fnegElem = AStanza.firstElement("si",NS_STREAM_INITIATION).firstChildElement("feature");
				while (!fnegElem.isNull() && fnegElem.namespaceURI()!=NS_FEATURENEG)
					fnegElem = fnegElem.nextSiblingElement("feature");

				QDomElement formElem = fnegElem.firstChildElement("x");
				while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
					formElem = formElem.nextSiblingElement("x");
				IDataForm form = FDataForms->dataForm(formElem);

				int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,form.fields);
				QString smethod = index>=0 ? form.fields.at(index).value.toString() : QString::null;
				if (FMethods.contains(smethod) && FDataForms->isSubmitValid(params.features,form))
				{
					LOG_STRM_INFO(AStreamJid,QString("Received stream initiation response, sid=%1").arg(sid));
					sprofile->dataStreamResponce(sid,AStanza,smethod);
				}
				else
				{
					LOG_STRM_WARNING(AStreamJid,QString("Failed to receive stream initiation response, sid=%1: Invalid response").arg(sid));
					sprofile->dataStreamError(sid,XmppError(IERR_DATASTREAMS_STREAM_INVALID_INIT_RESPONCE));
				}
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_WARNING(AStreamJid,QString("Failed to receive stream initiation response, sid=%1: %2").arg(sid,err.errorMessage()));
				sprofile->dataStreamError(sid,err);
			}
		}
		else
		{
			REPORT_ERROR("Failed to receive stream initiation response: Profile not found");
		}
	}
}

QList<QString> DataStreamsManger::methods() const
{
	return FMethods.keys();
}

IDataStreamMethod *DataStreamsManger::method(const QString &AMethodNS) const
{
	return FMethods.value(AMethodNS,NULL);
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
	return FProfiles.value(AProfileNS,NULL);
}

void DataStreamsManger::insertProfile(IDataStreamProfile *AProfile)
{
	if (AProfile!=NULL && !FProfiles.contains(AProfile->profileNS()) && !FProfiles.values().contains(AProfile))
	{
		LOG_DEBUG(QString("Stream profile inserted, ns=%1").arg(AProfile->profileNS()));
		FProfiles.insert(AProfile->profileNS(),AProfile);
		emit profileInserted(AProfile);
	}
}

void DataStreamsManger::removeProfile(IDataStreamProfile *AProfile)
{
	if (FProfiles.values().contains(AProfile))
	{
		LOG_DEBUG(QString("Stream profile removed, ns=%1").arg(AProfile->profileNS()));
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

QString DataStreamsManger::settingsProfileName(const QUuid &AProfileId) const
{
	return Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,AProfileId.toString()).value("name").toString();
}

OptionsNode DataStreamsManger::settingsProfileNode(const QUuid &AProfileId, const QString &AMethodNS) const
{
	return Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,AProfileId.toString()).node("method",AMethodNS);
}

void DataStreamsManger::insertSettingsProfile(const QUuid &AProfileId, const QString &AName)
{
	if (!AProfileId.isNull() && !AName.isEmpty())
	{
		Options::node(OPV_DATASTREAMS_SPROFILE_ITEM,AProfileId.toString()).setValue(AName,"name");
		emit settingsProfileInserted(AProfileId, AName);
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

bool DataStreamsManger::initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, const QList<QString> &AMethods, int ATimeout)
{
	if (FStanzaProcessor && FDataForms && !AStreamId.isEmpty() && !FStreams.contains(AStreamId) && !FMethods.isEmpty())
	{
		IDataStreamProfile *sprofile = FProfiles.value(AProfileNS,NULL);
		if (sprofile)
		{
			Stanza request("iq");
			request.setTo(AContactJid.full()).setType("set").setId(FStanzaProcessor->newId());
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

			if (!field.options.isEmpty() && sprofile->requestDataStream(AStreamId,request))
			{
				IDataForm form;
				form.type = DATAFORM_TYPE_FORM;
				form.fields.append(field);
				QDomElement negElem = siElem.appendChild(request.createElement("feature",NS_FEATURENEG)).toElement();
				FDataForms->xmlForm(form,negElem);
				if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,ATimeout))
				{
					StreamParams params;
					params.streamJid = AStreamJid;
					params.contactJid = AContactJid;
					params.requestId = request.id();
					params.profile = AProfileNS;
					params.features = form;
					FStreams.insert(AStreamId,params);
					LOG_STRM_INFO(AStreamJid,QString("Stream initiation request sent, to=%1, profile=%2, sid=%3").arg(AContactJid.full(),AProfileNS,AStreamId));
					return true;
				}
				else
				{
					LOG_STRM_WARNING(AStreamJid,QString("Failed to send stream initiation request"));
				}
			}
			else if (field.options.isEmpty())
			{
				LOG_WARNING(QString("Failed to request stream initiation: Methods not supported"));
			}
			else
			{
				LOG_WARNING(QString("Failed to request stream initiation: Profile=%1 not ready").arg(AProfileNS));
			}
		}
		else
		{
			LOG_WARNING(QString("Failed to request stream initiation: Profile=%1 not found").arg(AProfileNS));
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to request stream initiation: Invalid params");
	}
	return false;
}

bool DataStreamsManger::acceptStream(const QString &AStreamId, const QString &AMethodNS)
{
	if (FStanzaProcessor && FDataForms && FStreams.contains(AStreamId) && FMethods.contains(AMethodNS))
	{
		StreamParams params = FStreams.value(AStreamId);
		IDataStreamProfile *sprofile = FProfiles.value(params.profile,NULL);
		int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,params.features.fields);
		if (sprofile && index>=0 && FDataForms->isOptionValid(params.features.fields.at(index).options,AMethodNS))
		{
			Stanza response("iq");
			response.setType("result").setId(params.requestId).setTo(params.contactJid.full());
			QDomElement siElem = response.addElement("si",NS_STREAM_INITIATION);
			if (sprofile->responceDataStream(AStreamId,response))
			{
				QDomElement negElem = siElem.appendChild(response.createElement("feature",NS_FEATURENEG)).toElement();
				params.features.fields[index].value = AMethodNS;
				FDataForms->xmlForm(FDataForms->dataSubmit(params.features),negElem);
				if (FStanzaProcessor->sendStanzaOut(params.streamJid,response))
				{
					LOG_STRM_DEBUG(params.streamJid,QString("Stream accept response sent, sid=%1, method=%2").arg(AStreamId,AMethodNS));
					FStreams.remove(AStreamId);
					return true;
				}
				else
				{
					LOG_STRM_WARNING(params.streamJid,QString("Failed to send stream accept response, sid=%1").arg(AStreamId));
				}
			}
			else
			{
				LOG_STRM_WARNING(params.streamJid,QString("Failed to accept stream, sid=%1: Profile not ready").arg(AStreamId));
			}
		}
		else if (sprofile == NULL)
		{
			REPORT_ERROR("Failed to accept stream: Profile not found");
		}
		else
		{
			REPORT_ERROR("Failed to accept stream: Method not supported");
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to accept stream: Invalid params");
	}
	return false;
}

bool DataStreamsManger::rejectStream(const QString &AStreamId, const XmppStanzaError &AError)
{
	if (FStanzaProcessor && FStreams.contains(AStreamId))
	{
		StreamParams params = FStreams.take(AStreamId);
		
		Stanza error("iq");
		error.setId(params.requestId).setFrom(params.contactJid.full());
		error = FStanzaProcessor->makeReplyError(error,AError);

		if (FStanzaProcessor->sendStanzaOut(params.streamJid,error))
		{
			LOG_STRM_INFO(params.streamJid,QString("Stream reject response sent, sid=%1: %2").arg(AStreamId,AError.errorMessage()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(params.streamJid,QString("Failed to send stream reject response, sid=%1").arg(AStreamId));
		}
	}
	else if (FStanzaProcessor)
	{
		REPORT_ERROR("Failed to reject stream: Invalid params");
	}
	return false;
}

QString DataStreamsManger::streamIdByRequestId(const QString &ARequestId) const
{
	for (QMap<QString, StreamParams>::const_iterator it = FStreams.constBegin(); it!=FStreams.constEnd(); ++it)
		if (it->requestId == ARequestId)
			return it.key();
	return QString::null;
}

void DataStreamsManger::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	QMap<QString, StreamParams>::iterator it = FStreams.begin();
	while (it != FStreams.end())
	{
		if (it->streamJid == AXmppStream->streamJid())
		{
			IDataStreamProfile *sprofile = FProfiles.value(it->profile,NULL);
			if (sprofile)
				sprofile->dataStreamError(it.key(),XmppStanzaError(XmppStanzaError::EC_RECIPIENT_UNAVAILABLE));
			it = FStreams.erase(it);
		}
		else
		{
			++it;
		}
	}
}

Q_EXPORT_PLUGIN2(plg_datastreamsmanager, DataStreamsManger);
