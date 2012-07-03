#include "datastreamsmanager.h"

#define ERC_BAD_PROFILE               "bad-profile"
#define ERC_NO_VALID_STREAMS          "no-valid-streams"

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

bool DataStreamsManger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
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

	return FStanzaProcessor!=NULL && FDataForms!=NULL;
}


bool DataStreamsManger::initObjects()
{
	XmppError::registerErrorString(NS_STREAM_INITIATION,ERC_BAD_PROFILE,tr("The profile is not understood or invalid"));
	XmppError::registerErrorString(NS_STREAM_INITIATION,ERC_NO_VALID_STREAMS,tr("None of the available streams are acceptable"));

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
			if (index>=0)
				foreach(IDataOption option, form.fields.at(index).options)
					if (FMethods.contains(option.value))
						smethods.append(option.value);

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
						XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
						err.setErrorText(tr("Invalid profile settings"));
						Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
						FStanzaProcessor->sendStanzaOut(AStreamJid,error);
					}
				}
				else
				{
					XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
					err.setErrorText(tr("Stream with same ID already exists"));
					Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
					FStanzaProcessor->sendStanzaOut(AStreamJid,error);
				}
			}
			else
			{
				XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
				err.setAppCondition(NS_STREAM_INITIATION,ERC_NO_VALID_STREAMS);
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			}
		}
		else
		{
			XmppStanzaError err(XmppStanzaError::EC_BAD_REQUEST);
			err.setAppCondition(NS_STREAM_INITIATION,ERC_BAD_PROFILE);
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,err);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
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
				sprofile->dataStreamResponce(sid,AStanza,smethod);
			}
			else if (sprofile)
			{
				sprofile->dataStreamError(sid,tr("Invalid stream initiation response"));
			}
		}
		else if (sprofile)
		{
			sprofile->dataStreamError(sid,XmppStanzaError(AStanza).errorMessage());
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
		FMethods.insert(AMethod->methodNS(),AMethod);
		emit methodInserted(AMethod);
	}
}

void DataStreamsManger::removeMethod(IDataStreamMethod *AMethod)
{
	if (FMethods.values().contains(AMethod))
	{
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
		FProfiles.insert(AProfile->profileNS(),AProfile);
		emit profileInserted(AProfile);
	}
}

void DataStreamsManger::removeProfile(IDataStreamProfile *AProfile)
{
	if (FProfiles.values().contains(AProfile))
	{
		FProfiles.remove(FProfiles.key(AProfile));
		emit profileRemoved(AProfile);
	}
}

QList<QUuid> DataStreamsManger::settingsProfiles() const
{
	QList<QUuid> sprofiles;
	sprofiles.append(QUuid().toString());
	foreach(QString sprofile, Options::node(OPV_DATASTREAMS_ROOT).childNSpaces("settings-profile"))
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

bool DataStreamsManger::initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS,
                                   const QList<QString> &AMethods, int ATimeout)
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
			foreach(QString smethod, AMethods)
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
					return true;
				}
			}
		}
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
					FStreams.remove(AStreamId);
					return true;
				}
			}
		}
	}
	return false;
}

bool DataStreamsManger::rejectStream(const QString &AStreamId, const QString &AError)
{
	if (FStanzaProcessor && FStreams.contains(AStreamId))
	{
		StreamParams params = FStreams.take(AStreamId);
		
		XmppStanzaError err(XmppStanzaError::EC_FORBIDDEN);
		err.setErrorText(AError);

		Stanza error("iq");
		error.setId(params.requestId).setFrom(params.contactJid.full());
		error = FStanzaProcessor->makeReplyError(error,err);

		return FStanzaProcessor->sendStanzaOut(params.streamJid,error);
	}
	return false;
}

QString DataStreamsManger::streamIdByRequestId(const QString &ARequestId) const
{
	for (QMap<QString, StreamParams>::const_iterator it = FStreams.constBegin(); it!=FStreams.constEnd(); it++)
		if (it->requestId == ARequestId)
			return it.key();
	return QString::null;
}

void DataStreamsManger::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	QMap<QString, StreamParams>::iterator it = FStreams.begin();
	while (it!=FStreams.end())
	{
		if (it->streamJid == AXmppStream->streamJid())
		{
			IDataStreamProfile *sprofile = FProfiles.value(it->profile,NULL);
			if (sprofile)
				sprofile->dataStreamError(it.key(),XmppStanzaError(XmppStanzaError::EC_RECIPIENT_UNAVAILABLE).errorMessage());
			it = FStreams.erase(it);
		}
		else
		{
			it++;
		}
	}
}

Q_EXPORT_PLUGIN2(plg_datastreamsmanager, DataStreamsManger);
