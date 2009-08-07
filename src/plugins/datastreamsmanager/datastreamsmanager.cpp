#include "datastreamsmanager.h"

#define ERC_BAD_PROFILE       "bad-profile"
#define ERC_NO_VALID_STREAMS  "no-valid-streams"

#define DFV_STREAM_METHOD     "stream-method"

#define SHC_INIT_STREAM       "/iq[@type='set']/si[@xmlns='" NS_STREAM_INITIATION "']"

DataStreamsManger::DataStreamsManger()
{
  FDataForms = NULL;
  FDiscovery = NULL;
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
}

DataStreamsManger::~DataStreamsManger()
{

}

void DataStreamsManger::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Initiate a data stream between any two XMPP entities");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Data Streams Manager");
  APluginInfo->uid = DATASTREAMSMANAGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(DATAFORMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool DataStreamsManger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
  {
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());
  }
  
  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
  {
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool DataStreamsManger::initObjects()
{
  if (FStanzaProcessor)
  {
    FSHIInitStream = FStanzaProcessor->insertHandler(this,SHC_INIT_STREAM,IStanzaProcessor::DirectionIn,SHP_DEFAULT);
  }

  if (FDiscovery)
  {
    IDiscoFeature dfeature;
    dfeature.var = NS_STREAM_INITIATION;
    dfeature.active = true;
    dfeature.name = tr("Data Streams Initiation");
    dfeature.description = tr("Initiate a data stream between any two XMPP entities");
    FDiscovery->insertDiscoFeature(dfeature);
  }

  ErrorHandler::addErrorItem(ERC_NO_VALID_STREAMS,ErrorHandler::CANCEL,ErrorHandler::BAD_REQUEST,
    tr("None of the available streams are acceptable"),NS_STREAM_INITIATION);
  ErrorHandler::addErrorItem(ERC_BAD_PROFILE,ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("The profile is not understood or invalid"),NS_STREAM_INITIATION);

  return true;
}

bool DataStreamsManger::editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &AAccept)
{
  Q_UNUSED(AHandlerId);
  Q_UNUSED(AStreamJid);
  Q_UNUSED(AStanza);
  Q_UNUSED(AAccept);
  return false;
}

bool DataStreamsManger::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
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

        StreamParams params;
        params.streamJid = AStreamJid;
        params.contactJid = AStanza.from();
        params.requestId = AStanza.id();
        params.profile = siElem.attribute("profile");
        params.features = form;
        FStreams.insert(sid,params);

        if (sid.isEmpty() || !sprofile->processStreamRequest(sid,AStanza,smethods))
        {
          FStreams.remove(sid);
          Stanza error = errorStanza(AStanza.from(),AStanza.id(),"bad-request",EHN_DEFAULT,tr("Invalid profile settings"));
          FStanzaProcessor->sendStanzaOut(AStreamJid,error);
        }
      }
      else
      {
        Stanza error = errorStanza(AStanza.from(),AStanza.id(),ERC_NO_VALID_STREAMS,NS_STREAM_INITIATION);
        FStanzaProcessor->sendStanzaOut(AStreamJid,error);
      }
    }
    else
    {
      Stanza error = errorStanza(AStanza.from(),AStanza.id(),ERC_BAD_PROFILE,NS_STREAM_INITIATION);
      FStanzaProcessor->sendStanzaOut(AStreamJid,error);
    }
  }
  return false;
}

void DataStreamsManger::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
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
        sprofile->precessStreamResponce(sid,AStanza,smethod);
      }
      else if (sprofile)
      {
        sprofile->processStreamError(sid,tr("Invalid stream initiation responce"));
      }
    }
    else if (sprofile)
    {
      sprofile->processStreamError(sid,ErrorHandler(AStanza.element()).message());
    }
  }
}

void DataStreamsManger::iqStanzaTimeOut(const QString &AId)
{
  QString sid = streamIdByRequestId(AId);
  if (FDataForms && FStreams.contains(sid))
  {
    FStreams.remove(sid);
    IDataStreamProfile *sprofile = FProfiles.value(sid,NULL);
    if (sprofile)
      sprofile->processStreamError(sid,ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
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

void DataStreamsManger::insertMethod(IDataStreamMethod *AMethod, const QString &AMethodNS)
{
  if (AMethod!=NULL && !AMethodNS.isEmpty() && !FMethods.contains(AMethodNS))
  {
    FMethods.insert(AMethodNS,AMethod);
    emit methodInserted(AMethod,AMethodNS);
  }
}

void DataStreamsManger::removeMethod(IDataStreamMethod *AMethod, const QString &AMethodNS)
{
  if (FMethods.value(AMethodNS) == AMethod)
  {
    FMethods.remove(AMethodNS);
    emit methodRemoved(AMethod,AMethodNS);
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

void DataStreamsManger::insertProfile(IDataStreamProfile *AProfile, const QString &AProfileNS)
{
  if (!AProfileNS.isEmpty() && AProfile!=NULL && !FProfiles.contains(AProfileNS))
  {
    FProfiles.insert(AProfileNS,AProfile);
    emit profileInserted(AProfile,AProfileNS);
  }
}

void DataStreamsManger::removeProfile(IDataStreamProfile *AProfile, const QString &AProfileNS)
{
  if (FProfiles.value(AProfileNS) == AProfile)
  {
    FProfiles.remove(AProfileNS);
    emit profileRemoved(AProfile,AProfileNS);
  }
}

bool DataStreamsManger::initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfile, 
                             const QList<QString> &AMethods, int ATimeout)
{
  if (FStanzaProcessor && FDataForms && !AStreamId.isEmpty() && !FStreams.contains(AStreamId) && !FMethods.isEmpty())
  {
    IDataStreamProfile *sprofile = FProfiles.value(AProfile,NULL);
    if (sprofile)
    {
      Stanza request("iq");
      request.setTo(AContactJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
      QDomElement siElem = request.addElement("si",NS_STREAM_INITIATION);
      siElem.setAttribute("id",AStreamId);
      siElem.setAttribute("profile",AProfile);

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
      if (!field.options.isEmpty() && sprofile->insertRequestData(AStreamId,request))
      {
        IDataForm form;
        form.type = DATAFORM_TYPE_FORM;
        form.fields.append(field);
        QDomElement negElem = siElem.appendChild(request.createElement("feature",NS_FEATURENEG)).toElement();
        FDataForms->xmlForm(form,negElem);
        if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,ATimeout))
        {
          StreamParams params;
          params.streamJid = AStreamJid;
          params.contactJid = AContactJid;
          params.requestId = request.id();
          params.profile = AProfile;
          params.features = form;
          FStreams.insert(AStreamId,params);
          return true;
        }
      }
    }
  }
  return false;
}

bool DataStreamsManger::acceptStream(const QString &AStreamId, const QString &AMethod)
{
  if (FStanzaProcessor && FDataForms && FStreams.contains(AStreamId) && FMethods.contains(AMethod))
  {
    StreamParams params = FStreams.value(AStreamId);
    IDataStreamProfile *sprofile = FProfiles.value(params.profile,NULL);
    int index = FDataForms->fieldIndex(DFV_STREAM_METHOD,params.features.fields);
    if (sprofile && index>=0 && FDataForms->isOptionValid(params.features.fields.at(index).options,AMethod))
    {
      Stanza responce("iq");
      responce.setTo(params.contactJid.eFull()).setType("result").setId(params.requestId);
      QDomElement siElem = responce.addElement("si",NS_STREAM_INITIATION);
      if (sprofile->insertResponceData(AStreamId,responce))
      {
        QDomElement negElem = siElem.appendChild(responce.createElement("feature",NS_FEATURENEG)).toElement();
        params.features.fields[index].value = AMethod;
        FDataForms->xmlForm(FDataForms->dataSubmit(params.features),negElem);
        if (FStanzaProcessor->sendStanzaOut(params.streamJid,responce))
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
  if (FStreams.contains(AStreamId))
  {
    StreamParams params = FStreams.take(AStreamId);
    Stanza error = errorStanza(params.contactJid,params.requestId,"forbidden",EHN_DEFAULT,AError);
    FStanzaProcessor->sendStanzaOut(params.streamJid,error);
    return true;
  }
  return false;
}

Stanza DataStreamsManger::errorStanza(const Jid &AContactJid, const QString &ARequestId, const QString &ACondition, 
                                const QString &AErrNS, const QString &AText) const
{
  Stanza error("iq");
  error.setTo(AContactJid.eFull()).setType("error").setId(ARequestId);
  QDomElement errElem = error.addElement("error");
  errElem.setAttribute("code",ErrorHandler::codeByCondition(ACondition,AErrNS));
  errElem.setAttribute("type",ErrorHandler::typeByCondition(ACondition,AErrNS));
  errElem.appendChild(error.createElement(ACondition,AErrNS));
  if (AErrNS != EHN_DEFAULT)
    errElem.appendChild(error.createElement("bad-request",EHN_DEFAULT));
  else if (!AText.isEmpty())
    errElem.appendChild(error.createElement("text",EHN_DEFAULT)).appendChild(error.createTextNode(AText));
  return error;
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
  while(it!=FStreams.end())
  {
    if (it->streamJid == AXmppStream->jid())
    {
      IDataStreamProfile *sprofile = FProfiles.value(it->profile,NULL);
      if (sprofile)
        sprofile->processStreamError(it.key(),ErrorHandler(ErrorHandler::GONE).message());
      it = FStreams.erase(it);
    }
    else
      it++;
  }
}

Q_EXPORT_PLUGIN2(DataStreamsManagerPlugin, DataStreamsManger);
