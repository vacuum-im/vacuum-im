#include "sessionnegotiation.h"

#include <QUuid>
#include <QCryptographicHash>
#include <definitions/namespaces.h>
#include <definitions/dataformtypes.h>
#include <definitions/sessionnegotiatororders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

#define SHC_STANZA_SESSION            "/message/feature[@xmlns='"NS_FEATURENEG"']"

#define SFP_DISCLOSURE                "disclosure"
#define SFP_MULTISESSION              "multisession"

#define SFV_DISCLOSURE_NEVER          "never"
#define SFV_DISCLOSURE_DISABLED       "disabled"
#define SFV_DISCLOSURE_ENABLED        "enabled"

#define ADR_STREAM_JID                Action::DR_Parametr1
#define ADR_CONTACT_JID               Action::DR_Parametr2
#define ADR_SESSION_FIELD             Action::DR_Parametr3

SessionNegotiation::SessionNegotiation()
{
	FDataForms = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
	FPresenceManager = NULL;
	FNotifications = NULL;
}

SessionNegotiation::~SessionNegotiation()
{

}

void SessionNegotiation::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Jabber Session Manager");
	APluginInfo->description = tr("Allows to set the session between two entities, which explains the rules of the exchange of XMPP stanzas");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SessionNegotiation::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		IXmppStreamManager *xmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (xmppStreamManager)
		{
			connect(xmppStreamManager->instance(), SIGNAL(streamOpened(IXmppStream *)), SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(xmppStreamManager->instance(), SIGNAL(streamAboutToClose(IXmppStream *)), SLOT(onXmppStreamAboutToClose(IXmppStream *)));
			connect(xmppStreamManager->instance(), SIGNAL(streamClosed(IXmppStream *)), SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoRecieved(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0,NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if (FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
		}
	}

	return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool SessionNegotiation::initObjects()
{
	if (FDiscovery)
	{
		registerDiscoFeatures();
		//FDiscovery->insertFeatureHandler(NS_STANZA_SESSION,this,DFO_DEFAULT);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_SESSION_NEGOTIATION;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SNEGOTIATION);
		notifyType.title = tr("When receiving session negotiation request");
		notifyType.kindMask = INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_SESSION_NEGOTIATION,notifyType);
	}
	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_SESSION_NEGOTIATION);
	}
	insertNegotiator(this,SNO_DEFAULT);
	return true;
}

bool SessionNegotiation::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHISession.value(AStreamJid) == AHandlerId)
	{
		Jid contactJid = AStanza.from();
		QString sessionId = AStanza.firstElement("thread").text();

		QDomElement featureElem = AStanza.firstElement("feature",NS_FEATURENEG);
		QDomElement formElem = featureElem.firstChildElement("x");
		while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
			formElem = formElem.nextSiblingElement("x");

		if (!sessionId.isEmpty() && !formElem.isNull())
		{
			LOG_STRM_INFO(AStreamJid,QString("Received stanza session data from=%1, sid=%2").arg(AStanza.from(),sessionId));

			IStanzaSession &session = FSessions[AStreamJid][contactJid];
			IStanzaSession bareSession = findSession(AStreamJid,contactJid.bare());
			if (session.sessionId!=sessionId && bareSession.sessionId==sessionId)
			{
				session = bareSession;
				session.contactJid = contactJid;
				removeSession(bareSession);
			}

			FSuspended.remove(sessionId);
			closeAcceptDialog(session);

			QString stanzaType = AStanza.type();
			if (stanzaType.isEmpty() || stanzaType=="normal")
			{
				IDataForm form = FDataForms->dataForm(formElem);
				bool isAccept = FDataForms->fieldIndex(SESSION_FIELD_ACCEPT, form.fields) >= 0;

				if (isAccept && form.type==DATAFORM_TYPE_FORM)
				{
					terminateSession(AStreamJid,contactJid);
					session.streamJid = AStreamJid;
					session.contactJid = contactJid;
					session.sessionId = sessionId;
					processAccept(session,form);
				}
				else if (session.sessionId == sessionId)
				{
					bool isRenegotiate = FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE, form.fields) >= 0;
					bool isContinue = FDataForms->fieldIndex(SESSION_FIELD_CONTINUE, form.fields) >= 0;
					bool isTerminate = FDataForms->fieldIndex(SESSION_FIELD_TERMINATE, form.fields) >= 0;
					if (isAccept && session.status!=IStanzaSession::Active)
						processAccept(session,form);
					else if (isRenegotiate && (session.status==IStanzaSession::Active || session.status==IStanzaSession::Renegotiate))
						processRenegotiate(session,form);
					else if (isContinue && session.status==IStanzaSession::Active)
						processContinue(session,form);
					else if (isTerminate)
						processTerminate(session,form);
				}
				else if (session.status != IStanzaSession::Empty)
				{
					terminateSession(AStreamJid,contactJid);
				}
				else
				{
					removeSession(session);
				}
			}
			else if (stanzaType=="error" && session.sessionId==sessionId)
			{
				session.status = IStanzaSession::Error;
				session.error = XmppStanzaError(AStanza);

				session.errorFields.clear();
				QDomElement errorElem = AStanza.firstElement("error");
				QDomElement featureElem = errorElem.firstChildElement("feature");
				while (!featureElem.isNull() && featureElem.namespaceURI()!=NS_FEATURENEG)
					featureElem = featureElem.nextSiblingElement("feature");

				QDomElement fieldElem = featureElem.firstChildElement("field");
				while (!fieldElem.isNull())
				{
					if (fieldElem.hasAttribute("var"))
						session.errorFields.append(fieldElem.attribute("var"));
					fieldElem = fieldElem.nextSiblingElement("field");
				}

				LOG_STRM_INFO(AStreamJid,QString("Stanza session aborted by=%1, sid=%2: %2").arg(AStanza.from(),sessionId,session.error.condition()));
				emit sessionTerminated(session);
			}
			else if (session.status == IStanzaSession::Empty)
			{
				removeSession(session);
			}
			AAccept = true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to process stanza session data from=%1, sid=%2: Invalid params").arg(AStanza.from(),sessionId));
		}
	}
	return false;
}

bool SessionNegotiation::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	Q_UNUSED(AStreamJid); Q_UNUSED(AFeature); Q_UNUSED(ADiscoInfo);
	return false;
}

Action *SessionNegotiation::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature == NS_STANZA_SESSION)
	{
		Action *action = new Action(AParent);
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
		connect(action,SIGNAL(triggered(bool)),SLOT(onSessionActionTriggered(bool)));

		IStanzaSession session = findSession(AStreamJid,ADiscoInfo.contactJid);
		if (session.status==IStanzaSession::Empty || session.status==IStanzaSession::Terminate || session.status==IStanzaSession::Error)
		{
			action->setData(ADR_SESSION_FIELD,SESSION_FIELD_ACCEPT);
			action->setText(tr("Negotiate Session"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_SNEGOTIATION_INIT);
		}
		else
		{
			action->setData(ADR_SESSION_FIELD,SESSION_FIELD_TERMINATE);
			action->setText(tr("Terminate Session"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_SNEGOTIATION_TERMINATE);
		}
		return action;
	}
	return NULL;
}

IDataFormLocale SessionNegotiation::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_SESSION_NEGOTIATION)
	{
		locale.title = tr("Session Negotiation");
		locale.fields["accept"].label = tr("Accept the Invitation?");
		locale.fields["continue"].label = tr("Another Resource");
		locale.fields["disclosure"].label = tr("Disclosure of Content");
		locale.fields["http://jabber.org/protocol/chatstates"].label = tr("Enable Chat State Notifications?");
		locale.fields["http://jabber.org/protocol/xhtml-im"].label = tr("Enable XHTML-IM formatting?");
		locale.fields["language"].label = tr("Primary Written Language of the Chat");
		locale.fields["logging"].label = tr("Enable Message Loggings?");
		locale.fields["renegotiate"].label = tr("Renegotiate the Session?");
		locale.fields["security"].label = tr("Minimum Security Level");
		locale.fields["terminate"].label = tr("Terminate the Session?");
		locale.fields["urn:xmpp:receipts"].label = tr("Enable Message Receipts?");
	}
	return locale;
}

int SessionNegotiation::sessionInit(const IStanzaSession &ASession, IDataForm &ARequest)
{
	int result = ISessionNegotiator::Skip;

	if (ASession.status == IStanzaSession::Init)
	{
		IDataField multisession;
		multisession.var = SFP_MULTISESSION;
		multisession.type = DATAFIELD_TYPE_BOOLEAN;
		multisession.value = false;
		multisession.required = false;
		ARequest.fields.append(multisession);
		result = ISessionNegotiator::Auto;
	}

	return result;
}

int SessionNegotiation::sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit)
{
	Q_UNUSED(ASession);
	int result = ISessionNegotiator::Skip;

	int index = FDataForms->fieldIndex(SFP_MULTISESSION,ARequest.fields);
	if (index>=0)
	{
		if (ARequest.type == DATAFORM_TYPE_FORM)
		{
			IDataField multisession;
			multisession.var = SFP_MULTISESSION;
			multisession.type = DATAFIELD_TYPE_BOOLEAN;
			multisession.value = false;
			multisession.required = false;
			ASubmit.fields.append(multisession);
			result = ISessionNegotiator::Auto;
		}
		else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
		{
			result = ARequest.fields[index].value.toBool() ? ISessionNegotiator::Cancel : ISessionNegotiator::Auto;
		}
	}

	return result;
}

int SessionNegotiation::sessionApply(const IStanzaSession &ASession)
{
	Q_UNUSED(ASession);
	return ISessionNegotiator::Auto;
}

void SessionNegotiation::sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm)
{
	Q_UNUSED(ASession);
	int index = FDataForms->fieldIndex(SFP_MULTISESSION,AForm.fields);
	if (index>=0)
		AForm.fields[index].label = tr("Allow multiple sessions?");
}

bool SessionNegotiation::isReady(const Jid &AStreamJid) const
{
	return FSHISession.contains(AStreamJid);
}

IStanzaSession SessionNegotiation::findSession(const QString &ASessionId) const
{
	foreach(const Jid &streamJid, FSessions.keys())
		foreach(const IStanzaSession &session, FSessions.value(streamJid))
			if (session.sessionId == ASessionId)
				return session;
	return IStanzaSession();
}

IStanzaSession SessionNegotiation::findSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FSessions.value(AStreamJid).value(AContactJid);
}

QList<IStanzaSession> SessionNegotiation::findSessions(const Jid &AStreamJid, int AStatus) const
{
	QList<IStanzaSession> sessions;
	foreach(const IStanzaSession &session, FSessions.value(AStreamJid).values())
		if (session.status == AStatus)
			sessions.append(session);
	return sessions;
}

int SessionNegotiation::initSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (isReady(AStreamJid))
	{
		IStanzaSession &session = FSessions[AStreamJid][AContactJid];
		if (AStreamJid != AContactJid &&
			session.status != IStanzaSession::Accept &&
			session.status != IStanzaSession::Pending &&
			session.status != IStanzaSession::Apply &&
			session.status != IStanzaSession::Renegotiate &&
			session.status != IStanzaSession::Continue)
		{
			bool isRenegotiate = session.status==IStanzaSession::Active;
			IDataForm request = defaultForm(isRenegotiate ? SESSION_FIELD_RENEGOTIATE : SESSION_FIELD_ACCEPT);
			request.type.clear();

			if (!isRenegotiate)
			{
				session.status = IStanzaSession::Init;
				session.sessionId = QUuid::createUuid().toString();
				session.streamJid = AStreamJid;
				session.contactJid = AContactJid;
				session.form = IDataForm();
				session.error = XmppStanzaError::null;
				session.errorFields.clear();
				LOG_STRM_INFO(AStreamJid,QString("Initializing stanza session, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
			}
			else
			{
				session.status = IStanzaSession::Renegotiate;
				LOG_STRM_INFO(AStreamJid,QString("Renegotiating stanza session, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
			}

			int result = 0;
			foreach(ISessionNegotiator *negotiator, FNegotiators)
				result = result | negotiator->sessionInit(session,request);

			if (!isRenegotiate && FDiscovery && !FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_STANZA_SESSION))
			{
				bool infoRequested = !FDiscovery->hasDiscoInfo(AStreamJid,AContactJid) ? FDiscovery->requestDiscoInfo(AStreamJid,AContactJid) : false;
				if (!infoRequested)
				{
					LOG_STRM_WARNING(AStreamJid,QString("Failed to initialize stanza session, with=%1, sid=%2: Stanza sessions not supported").arg(AContactJid.full(),session.sessionId));
					session.status = IStanzaSession::Error;
					session.error = XmppStanzaError(XmppStanzaError::EC_SERVICE_UNAVAILABLE);
					emit sessionTerminated(session);
					return ISessionNegotiator::Cancel;
				}
				else
				{
					LOG_STRM_INFO(AStreamJid,QString("Requested stanza session support info from=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					session.status = IStanzaSession::Init;
					FSuspended.insert(session.sessionId,IDataForm());
					return ISessionNegotiator::Wait;
				}
			}
			else if ((result & ISessionNegotiator::Cancel) > 0)
			{
				if (!isRenegotiate)
				{
					LOG_STRM_INFO(AStreamJid,QString("Stanza session initialization canceled, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					session.status = IStanzaSession::Terminate;
					emit sessionTerminated(session);
				}
				else
				{
					LOG_STRM_INFO(AStreamJid,QString("Stanza session renegotiation canceled, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					terminateSession(AStreamJid,AContactJid);
				}
				return ISessionNegotiator::Cancel;
			}
			else if ((result & ISessionNegotiator::Manual) > 0)
			{
				if (!isRenegotiate)
				{
					LOG_STRM_INFO(AStreamJid,QString("Manually approving stanza session initialization, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					session.form = clearForm(request);
				}
				else
				{
					LOG_STRM_INFO(AStreamJid,QString("Manually approving stanza session renegotiation, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
				}
				localizeSession(session,request);
				showAcceptDialog(session,request);
				return ISessionNegotiator::Manual;
			}
			else if ((result & ISessionNegotiator::Auto) > 0)
			{
				if (!isRenegotiate)
				{
					LOG_STRM_INFO(AStreamJid,QString("Sending stanza session initialization request to=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					session.form = clearForm(request);
					session.status = IStanzaSession::Pending;
				}
				else
				{
					LOG_STRM_INFO(AStreamJid,QString("Sending stanza session renegotiation request to=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
					FRenegotiate.insert(session.sessionId,request);
				}
				request.type = DATAFORM_TYPE_FORM;
				localizeSession(session,request);
				request.title = isRenegotiate ? tr("Session renegotiation") : tr("Session negotiation");
				sendSessionData(session,request);
				return ISessionNegotiator::Auto;
			}
		}
		else if (AStreamJid != AContactJid)
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to initialize stanza session, with=%1, sid=%2: Invalid status=%3").arg(session.contactJid.full(),session.sessionId).arg(session.status));
		}
		else
		{
			REPORT_ERROR("Failed to initialize stanza session: Invalid params");
		}
	}
	else
	{
		REPORT_ERROR("Failed to initialize stanza session: Stream is not ready");
	}
	return ISessionNegotiator::Skip;
}

void SessionNegotiation::resumeSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FSuspended.contains(FSessions.value(AStreamJid).value(AContactJid).sessionId))
	{
		IStanzaSession &session = FSessions[AStreamJid][AContactJid];
		LOG_STRM_INFO(AStreamJid,QString("Resuming stanza session, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));

		IDataForm request = FSuspended.take(session.sessionId);
		if (session.status == IStanzaSession::Init)
			initSession(session.streamJid,session.contactJid);
		else if (session.status == IStanzaSession::Accept)
			processAccept(session,request);
		else if (session.status == IStanzaSession::Apply)
			processApply(session,request);
		else if (session.status == IStanzaSession::Renegotiate)
			processRenegotiate(session,request);
		else if (session.status == IStanzaSession::Continue)
			processContinue(session,request);
	}
	else
	{
		REPORT_ERROR("Failed to resume stanza session: Session not found");
	}
}

void SessionNegotiation::terminateSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FSessions.value(AStreamJid).contains(AContactJid))
	{
		IStanzaSession &session = FSessions[AStreamJid][AContactJid];
		if (session.status != IStanzaSession::Empty &&
			session.status != IStanzaSession::Init &&
			session.status != IStanzaSession::Terminate &&
			session.status != IStanzaSession::Error)
		{
			LOG_STRM_INFO(AStreamJid,QString("Terminating stanza session, with=%1, sid=%2").arg(AContactJid.full(),session.sessionId));
			IDataForm request = defaultForm(SESSION_FIELD_TERMINATE);
			request.type = DATAFORM_TYPE_SUBMIT;
			session.status = IStanzaSession::Terminate;
			sendSessionData(session,request);
			emit sessionTerminated(session);
		}
	}
}

void SessionNegotiation::showSessionParams(const Jid &AStreamJid, const Jid &AContactJid)
{
	IStanzaSession session = findSession(AStreamJid,AContactJid);
	if (FDataForms && !session.form.fields.isEmpty())
	{
		IDataForm form = session.form;
		form.type = DATAFORM_TYPE_RESULT;
		localizeSession(session,form);
		form = FDataForms->dataShowSubmit(form,form);
		IDataDialogWidget *widget = FDataForms->dialogWidget(form,NULL);
		widget->dialogButtons()->setStandardButtons(QDialogButtonBox::Ok);
		widget->instance()->show();
	}
}

void SessionNegotiation::insertNegotiator(ISessionNegotiator *ANegotiator, int AOrder)
{
	if (!FNegotiators.contains(AOrder,ANegotiator))
	{
		LOG_DEBUG(QString("Stanza session negotiator inserted, order=%1, address=%2").arg(AOrder).arg((quint64)ANegotiator));
		FNegotiators.insert(AOrder,ANegotiator);
	}
}

void SessionNegotiation::removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder)
{
	if (FNegotiators.contains(AOrder,ANegotiator))
	{
		LOG_DEBUG(QString("Stanza session negotiator removed, order=%1, address=%2").arg(AOrder).arg((quint64)ANegotiator));
		FNegotiators.remove(AOrder,ANegotiator);
	}
}

bool SessionNegotiation::sendSessionData(const IStanzaSession &ASession, const IDataForm &AForm) const
{
	if (FStanzaProcessor && FDataForms && !AForm.fields.isEmpty())
	{
		Stanza data("message");
		data.setType("normal").setTo(ASession.contactJid.full());
		data.addElement("thread").appendChild(data.createTextNode(ASession.sessionId));
		QDomElement featureElem = data.addElement("feature",NS_FEATURENEG);
		IDataForm form = AForm;
		form.pages.clear();
		FDataForms->xmlForm(form,featureElem);
		if (FStanzaProcessor->sendStanzaOut(ASession.streamJid,data))
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session data sent to=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(ASession.streamJid,QString("Failed to send stanza session data to=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to send stanza session data: Form fields is empty");
	}
	return false;
}

bool SessionNegotiation::sendSessionError(const IStanzaSession &ASession, const IDataForm &ARequest) const
{
	if (FStanzaProcessor && FDataForms && !ASession.error.isNull())
	{
		Stanza error("message");
		error.setFrom(ASession.contactJid.full());
		error = FStanzaProcessor->makeReplyError(error,ASession.error);
		error.addElement("thread").appendChild(error.createTextNode(ASession.sessionId));
		
		IDataForm request = ARequest;
		request.pages.clear();

    QDomElement featureElem = error.addElement("feature",NS_FEATURENEG).toElement();
    FDataForms->xmlForm(request,featureElem);

		if (!ASession.errorFields.isEmpty())
		{
			QDomElement featureElem = error.firstElement("error").appendChild(error.createElement("feature",NS_FEATURENEG)).toElement();
			foreach(const QString &var, ASession.errorFields)
				featureElem.appendChild(error.createElement("field")).toElement().setAttribute("var",var);
		}
		if (FStanzaProcessor->sendStanzaOut(ASession.streamJid,error))
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session abort sent to=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(ASession.streamJid,QString("Failed to send stanza session abort to=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		REPORT_ERROR("Failed to send stanza session abort: Error is empty");
	}
	return false;
}

void SessionNegotiation::processAccept(IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (ARequest.type == DATAFORM_TYPE_FORM)
	{
		ASession.status = IStanzaSession::Accept;
		ASession.form = clearForm(ARequest);

		IDataForm submit = defaultForm(SESSION_FIELD_ACCEPT);
		submit.type = DATAFORM_TYPE_SUBMIT;

		int result = 0;
		foreach(ISessionNegotiator *negotiator, FNegotiators)
			result = result | negotiator->sessionAccept(ASession,ARequest,submit);

		if (!FDataForms->isSubmitValid(ARequest,submit))
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Failed to accept stanza session request, with=%1, sid=%2: Required feature not supported").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Error;
			ASession.error = XmppStanzaError(XmppStanzaError::EC_FEATURE_NOT_IMPLEMENTED);
			ASession.errorFields = unsubmitedFields(ARequest,submit,true);
			sendSessionError(ASession,ARequest);
		}
		else if ((result & ISessionNegotiator::Cancel) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session request not accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Terminate;
			submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
			updateFields(IDataForm(),submit,false,true);
			sendSessionData(ASession,submit);
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session request accept suspended, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			FSuspended.insert(ASession.sessionId,ARequest);
		}
		else if ((result & ISessionNegotiator::Manual) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Manually accepting stanza session request, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			updateFields(submit,ASession.form,false,true);
			IDataForm request = ASession.form;
			request.pages = submit.pages;
			localizeSession(ASession,request);
			showAcceptDialog(ASession,request);
		}
		else
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session request accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			updateFields(submit,ASession.form,false,true);
			processApply(ASession,submit);
		}
	}
	else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
	{
		if (FDataForms->fieldValue(SESSION_FIELD_ACCEPT,ARequest.fields).toBool())
		{
			ASession.status = IStanzaSession::Accept;

			IDataForm submit = defaultForm(SESSION_FIELD_ACCEPT);
			submit.type = DATAFORM_TYPE_RESULT;

			int result = 0;
			foreach(ISessionNegotiator *negotiator, FNegotiators)
				result = result | negotiator->sessionAccept(ASession,ARequest,submit);

			if (!FDataForms->isSubmitValid(ASession.form,ARequest))
			{
				LOG_STRM_WARNING(ASession.streamJid,QString("Failed to accept stanza session submit, with=%1, sid=%2: Required feature not submitted").arg(ASession.contactJid.full(),ASession.sessionId));
				ASession.status = IStanzaSession::Error;
				ASession.error = XmppStanzaError(XmppStanzaError::EC_NOT_ACCEPTABLE);
				ASession.errorFields = unsubmitedFields(ARequest,submit,true);
				sendSessionError(ASession,ARequest);
				emit sessionTerminated(ASession);
			}
			else if ((result & ISessionNegotiator::Cancel) > 0)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session submit not accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				ASession.status = IStanzaSession::Terminate;
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
				updateFields(IDataForm(),submit,false,true);
				sendSessionData(ASession,submit);
				updateFields(ARequest,ASession.form,false,false);
				updateFields(submit,ASession.form,true,false);
				emit sessionTerminated(ASession);
			}
			else if ((result & ISessionNegotiator::Wait) > 0)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session submit accept suspended, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				FSuspended.insert(ASession.sessionId,ARequest);
			}
			else if ((result & ISessionNegotiator::Manual) > 0)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Manually accepting stanza session submit, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				updateFields(ARequest,ASession.form,false,false);
				IDataForm request = ASession.form;
				request.pages = submit.pages;
				localizeSession(ASession,request);
				request = FDataForms->dataShowSubmit(request,ARequest);
				showAcceptDialog(ASession,request);
			}
			else
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session submit accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				updateFields(ARequest,ASession.form,false,false);
				processApply(ASession,submit);
			}
		}
		else
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session canceled by=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Terminate;
			updateFields(ARequest,ASession.form,true,false);
			emit sessionTerminated(ASession);
		}
	}
	else if (ARequest.type == DATAFORM_TYPE_RESULT)
	{
		if (FDataForms->fieldValue(SESSION_FIELD_ACCEPT,ARequest.fields).toBool())
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session activated, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Active;
			emit sessionActivated(ASession);
		}
		else
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session canceled by=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Terminate;
			updateFields(ARequest,ASession.form,true,false);
			emit sessionTerminated(ASession);
		}
	}
	else
	{
		LOG_STRM_WARNING(ASession.streamJid,QString("Failed to accept stanza session, with=%1, sid=%2: Invalid form type=%3").arg(ASession.contactJid.full(),ASession.sessionId,ARequest.type));
	}
}

void SessionNegotiation::processApply(IStanzaSession &ASession, const IDataForm &ASubmit)
{
	bool isAccept = FDataForms!=NULL ? FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,ASubmit.fields)>=0 : false;
	bool isRenegotiate = FDataForms!=NULL ? FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,ASubmit.fields)>=0 : false;

	if (isAccept || isRenegotiate)
	{
		ASession.status = IStanzaSession::Apply;

		int result = 0;
		foreach(ISessionNegotiator *negotiator, FNegotiators)
			result = result | negotiator->sessionApply(ASession);

		if ((result & ISessionNegotiator::Cancel) > 0)
		{
			if (isAccept)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session not applied, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				ASession.status = IStanzaSession::Terminate;
				IDataForm submit = ASubmit;
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
				sendSessionData(ASession,submit);
				emit sessionTerminated(ASession);
			}
			else if (ASubmit.type == DATAFORM_TYPE_SUBMIT)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation not applied, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				ASession.status = IStanzaSession::Active;
				IDataForm submit = ASubmit;
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
				sendSessionData(ASession,submit);
			}
			else
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session apply canceled, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				terminateSession(ASession.streamJid,ASession.contactJid);
			}
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			if (isAccept)
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session apply suspended, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			else
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation apply suspended, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			FSuspended.insert(ASession.sessionId,ASubmit);
		}
		else
		{
			if (isAccept)
			{
				ASession.status = ASubmit.type==DATAFORM_TYPE_RESULT ? IStanzaSession::Active : IStanzaSession::Pending;
				sendSessionData(ASession,ASubmit);
				if (ASession.status == IStanzaSession::Active)
				{
					LOG_STRM_INFO(ASession.streamJid,QString("Stanza session applied and activated, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
					emit sessionActivated(ASession);
				}
				else
				{
					LOG_STRM_INFO(ASession.streamJid,QString("Stanza session applied, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				}
			}
			else
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation applied and activated, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				ASession.status = IStanzaSession::Active;
				if (ASubmit.type == DATAFORM_TYPE_SUBMIT)
					sendSessionData(ASession,ASubmit);
				emit sessionActivated(ASession);
			}
		}
	}
}

void SessionNegotiation::processRenegotiate(IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (ARequest.type == DATAFORM_TYPE_FORM)
	{
		ASession.status = IStanzaSession::Renegotiate;

		IDataForm submit = defaultForm(SESSION_FIELD_RENEGOTIATE);
		submit.type = DATAFORM_TYPE_SUBMIT;

		int result = 0;
		foreach(ISessionNegotiator *negotiator, FNegotiators)
			result = result | negotiator->sessionAccept(ASession,ARequest,submit);

		if (!FDataForms->isSubmitValid(ARequest,submit))
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Failed to renegotiate stanza session, with=%1, sid=%2: Required feature not supported").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Active;
			submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
			sendSessionData(ASession,submit);
		}
		else if ((result & ISessionNegotiator::Cancel) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation request not accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			ASession.status = IStanzaSession::Active;
			submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
			sendSessionData(ASession,submit);
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation request accept suspended, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			FSuspended.insert(ASession.sessionId,ARequest);
		}
		else if ((result & ISessionNegotiator::Manual) > 0)
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Manually accepting stanza session renegotiation request, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			IDataForm request = ARequest;
			request.pages = submit.pages;
			updateFields(submit,request,false,false);
			localizeSession(ASession,request);
			showAcceptDialog(ASession,request);
		}
		else
		{
			LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation request accepted, with=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
			updateFields(submit,ASession.form,false,false);
			processApply(ASession,submit);
		}
	}
	else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
	{
		if (FRenegotiate.contains(ASession.sessionId))
		{
			ASession.status = IStanzaSession::Renegotiate;

			IDataForm form = FRenegotiate.take(ASession.sessionId);
			if (!FDataForms->isSubmitValid(form,ARequest))
			{
				LOG_STRM_WARNING(ASession.streamJid,QString("Failed to accept stanza session renegotiation submit, with=%1, sid=%2: Required feature not submitted").arg(ASession.contactJid.full(),ASession.sessionId));
				terminateSession(ASession.streamJid,ASession.contactJid);
			}
			else if (!FDataForms->fieldValue(SESSION_FIELD_RENEGOTIATE,ARequest.fields).toBool())
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation canceled, by=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				terminateSession(ASession.streamJid,ASession.contactJid);
			}
			else
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session renegotiation accepted, by=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
				IDataForm submit = defaultForm(SESSION_FIELD_RENEGOTIATE);
				submit.type = DATAFORM_TYPE_RESULT;
				updateFields(ARequest,ASession.form,false,false);
				processApply(ASession,submit);
			}
		}
		else
		{
			LOG_STRM_WARNING(ASession.streamJid,QString("Failed to accept stanza session renegotiation submit, with=%1, sid=%2: Renegotiation not requested").arg(ASession.contactJid.full(),ASession.sessionId));
		}
	}
	else
	{
		LOG_STRM_WARNING(ASession.streamJid,QString("Failed to renegotiate stanza session with=%1, sid=%2: Invalid form type=%3").arg(ASession.contactJid.full(),ASession.sessionId,ARequest.type));
	}
}

void SessionNegotiation::processContinue(IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (ARequest.type == DATAFORM_TYPE_SUBMIT)
	{
		QString resource = FDataForms->fieldValue(SESSION_FIELD_CONTINUE,ARequest.fields).toString();
		if (!resource.isEmpty() && ASession.contactJid.resource()!=resource)
		{
			ASession.status = IStanzaSession::Continue;
			emit sessionTerminated(ASession);

			int result = 0;
			foreach(ISessionNegotiator *negotiator, FNegotiators)
				result = result | negotiator->sessionApply(ASession);

			if ((result & ISessionNegotiator::Cancel) > 0)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session continue not applied, with=%1, sid=%2, resource=%3").arg(ASession.contactJid.full(),ASession.sessionId,resource));
				ASession.status = IStanzaSession::Error;
				ASession.error = XmppStanzaError(XmppStanzaError::EC_NOT_ACCEPTABLE);
				sendSessionError(ASession,ARequest);
			}
			else if ((result & ISessionNegotiator::Wait) > 0)
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session continue suspended, with=%1, sid=%2, resource=%3").arg(ASession.contactJid.full(),ASession.sessionId,resource));
				FSuspended.insert(ASession.sessionId,ARequest);
			}
			else
			{
				LOG_STRM_INFO(ASession.streamJid,QString("Stanza session continue applied and activated, with=%1, sid=%2, resource=%3").arg(ASession.contactJid.full(),ASession.sessionId,resource));
				IDataForm submit = defaultForm(SESSION_FIELD_CONTINUE,resource);
				submit.type = DATAFORM_TYPE_RESULT;
				sendSessionData(ASession,submit);
				ASession.status = IStanzaSession::Active;
				ASession.contactJid.setResource(resource);
				emit sessionActivated(ASession);
			}
		}
		else
		{
			LOG_STRM_WARNING(ASession.streamJid,QString("Failed to continue stanza session, with=%1, sid=%2: Invalid resource=%3").arg(ASession.contactJid.full(),ASession.sessionId,resource));
		}
	}
	else
	{
		LOG_STRM_WARNING(ASession.streamJid,QString("Failed to continue stanza session, with=%1, sid=%2: Invalid form type=%3").arg(ASession.contactJid.full(),ASession.sessionId,ARequest.type));
	}
}

void SessionNegotiation::processTerminate(IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (ARequest.type == DATAFORM_TYPE_SUBMIT)
	{
		LOG_STRM_INFO(ASession.streamJid,QString("Stanza session terminated, by=%1, sid=%2").arg(ASession.contactJid.full(),ASession.sessionId));
		ASession.status = IStanzaSession::Terminate;
		emit sessionTerminated(ASession);
	}
	else
	{
		LOG_STRM_WARNING(ASession.streamJid,QString("Failed to terminate stanza session, with=%1, sid=%2: Invalid form type=%3").arg(ASession.contactJid.full(),ASession.sessionId,ARequest.type));
	}
}

void SessionNegotiation::showAcceptDialog(const IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (FDataForms)
	{
		IDataDialogWidget *dialog = FDialogs.value(ASession.streamJid).value(ASession.contactJid);
		if (!dialog)
		{
			dialog = FDataForms->dialogWidget(ARequest,NULL);
			dialog->instance()->installEventFilter(this);
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog->instance(),MNI_SNEGOTIATION,0,0,"windowIcon");
			dialog->dialogButtons()->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
			connect(dialog->instance(),SIGNAL(accepted()),SLOT(onAcceptDialogAccepted()));
			connect(dialog->instance(),SIGNAL(rejected()),SLOT(onAcceptDialogRejected()));
			connect(dialog->instance(),SIGNAL(dialogDestroyed(IDataDialogWidget *)),SLOT(onAcceptDialogDestroyed(IDataDialogWidget *)));
			FDialogs[ASession.streamJid].insert(ASession.contactJid,dialog);
		}
		else
		{
			dialog->setForm(ARequest);
		}

		if (FNotifications && !dialog->instance()->isActiveWindow())
		{
			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_SESSION_NEGOTIATION);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_SESSION_NEGOTIATION;
				notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SNEGOTIATION));
				notify.data.insert(NDR_TOOLTIP,tr("Session negotiation - %1").arg(ASession.contactJid.uFull()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Session negotiation"));
				notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(ASession.streamJid,ASession.contactJid));
				notify.data.insert(NDR_STREAM_JID,ASession.streamJid.full());
				notify.data.insert(NDR_CONTACT_JID,ASession.contactJid.full());
				notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(ASession.contactJid));
				notify.data.insert(NDR_POPUP_TEXT, notify.data.value(NDR_TOOLTIP).toString());
				notify.data.insert(NDR_SOUND_FILE, SDF_SNEGOTIATION_REQUEST);
				notify.data.insert(NDR_ALERT_WIDGET,(qint64)dialog->instance());
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)dialog->instance());
				int notifyId = FNotifications->appendNotification(notify);
				FDialogByNotify.insert(notifyId,dialog);
			}
		}
		else
		{
			dialog->instance()->show();
		}
	}
}

void SessionNegotiation::closeAcceptDialog(const IStanzaSession &ASession)
{
	IDataDialogWidget *dialog = FDialogs.value(ASession.streamJid).value(ASession.contactJid, NULL);
	if (dialog)
		dialog->instance()->deleteLater();
}

void SessionNegotiation::localizeSession(IStanzaSession &ASession, IDataForm &AForm) const
{
	AForm.title = tr("Session negotiation - %1").arg(ASession.contactJid.uFull());
	AForm.instructions = QStringList() << (AForm.type==DATAFORM_TYPE_FORM ? tr("Set desirable session settings.") : tr("Do you accept this session settings?"));

	if (FDataForms)
	{
		int index = FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,AForm.fields);
		if (index>=0)
			AForm.fields[index].label = tr("Accept this session?");

		index = FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,AForm.fields);
		if (index>=0)
			AForm.fields[index].label = tr("Renegotiate this session?");
	}

	foreach(ISessionNegotiator *negotiator, FNegotiators)
		negotiator->sessionLocalize(ASession,AForm);
}

void SessionNegotiation::removeSession(const IStanzaSession &ASession)
{
	if (FSessions.value(ASession.streamJid).contains(ASession.contactJid))
	{
		IStanzaSession sesssion = FSessions[ASession.streamJid].take(ASession.contactJid);
		FSuspended.remove(sesssion.sessionId);
		FRenegotiate.remove(sesssion.sessionId);
		closeAcceptDialog(sesssion);
	}
}

void SessionNegotiation::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.active = true;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SNEGOTIATION);
	dfeature.var = NS_STANZA_SESSION;
	dfeature.name = tr("Session Negotiation");
	dfeature.description = tr("Supports the negotiating of the stanza session between two XMPP entities");
	FDiscovery->insertDiscoFeature(dfeature);
}

void SessionNegotiation::updateFields(const IDataForm &ASourse, IDataForm &ADestination, bool AInsert, bool ARemove) const
{
	if (FDataForms)
	{
		const static QStringList reservedFields = QStringList()
		    << SESSION_FIELD_ACCEPT << SESSION_FIELD_CONTINUE << SESSION_FIELD_RENEGOTIATE
		    << SESSION_FIELD_TERMINATE << SESSION_FIELD_REASON << "FORM_TYPE";

		QStringList updated;
		foreach(const IDataField &field, ASourse.fields)
		{
			int index = FDataForms->fieldIndex(field.var,ADestination.fields);
			if (index >= 0)
				ADestination.fields[index].value = field.value;
			else if (AInsert && !reservedFields.contains(field.var))
				ADestination.fields.append(field);
			updated.append(field.var);
		}

		if (ARemove)
		{
			for (int index=0; index<ADestination.fields.count(); index++)
			{
				QString var = ADestination.fields.at(index).var;
				if (!reservedFields.contains(var) && !updated.contains(var))
					ADestination.fields.removeAt(index--);
			}
		}
	}
}

IDataForm SessionNegotiation::defaultForm(const QString &AActionVar, const QVariant &AValue) const
{
	IDataField form_type;
	form_type.var = "FORM_TYPE";
	form_type.type = DATAFIELD_TYPE_HIDDEN;
	form_type.value = DATA_FORM_SESSION_NEGOTIATION;
	form_type.required = false;

	IDataField actionField;
	actionField.var = AActionVar;
	actionField.type = AValue.type()==QVariant::Bool ? DATAFIELD_TYPE_BOOLEAN : DATAFIELD_TYPE_TEXTSINGLE;
	actionField.value = AValue;
	actionField.required = true;

	IDataForm form;
	form.fields.append(form_type);
	form.fields.append(actionField);
	form.pages.append(IDataLayout());

	return form;
}

IDataForm SessionNegotiation::clearForm(const IDataForm &AForm) const
{
	IDataForm form;
	form.type = AForm.type;
	foreach(const IDataField &field, AForm.fields)
	{
		IDataField clearField;
		clearField.type = field.type;
		clearField.var = field.var;
		clearField.value = field.value;
		clearField.required = field.required;
		foreach (const IDataOption &option, field.options)
		{
			IDataOption clearOption;
			clearOption.value = option.value;
			clearField.options.append(clearOption);
		}
		form.fields.append(clearField);
	}
	return form;
}

QStringList SessionNegotiation::unsubmitedFields(const IDataForm &ARequest, const IDataForm &ASubmit, bool ARequired) const
{
	QStringList fields;
	foreach(const IDataField &rField, ARequest.fields)
	{
		int index = FDataForms->fieldIndex(rField.var,ASubmit.fields);
		IDataField sField = index>=0 ? ASubmit.fields.at(index) : IDataField();
		if ((rField.required || !ARequired) && FDataForms->isFieldEmpty(sField))
			fields.append(rField.var);
	}
	return fields;
}

IStanzaSession &SessionNegotiation::dialogSession(IDataDialogWidget *ADialog)
{
	foreach(const Jid &streamJid, FDialogs.keys())
		if (FDialogs.value(streamJid).values().contains(ADialog))
			return FSessions[streamJid][FDialogs.value(streamJid).key(ADialog)];
	return FSessions[Jid::null][Jid::null];
}

bool SessionNegotiation::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		if (FNotifications)
		{
			int notifyId = FDialogByNotify.key(qobject_cast<IDataDialogWidget *>(AObject));
			FNotifications->removeNotification(notifyId);
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void SessionNegotiation::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor && FDataForms)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_STANZA_SESSION);
		FSHISession.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	emit sessionsOpened(AXmppStream->streamJid());
}

void SessionNegotiation::onXmppStreamAboutToClose(IXmppStream *AXmppStream)
{
	QList<IStanzaSession> sessions = FSessions.value(AXmppStream->streamJid()).values();
	foreach(const IStanzaSession &session, sessions)
	{
		terminateSession(session.streamJid,session.contactJid);
		removeSession(session);
	}
}

void SessionNegotiation::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor && FDataForms)
		FStanzaProcessor->removeStanzaHandle(FSHISession.take(AXmppStream->streamJid()));

	FDialogs.remove(AXmppStream->streamJid());
	FSessions.remove(AXmppStream->streamJid());

	emit sessionsClosed(AXmppStream->streamJid());
}

void SessionNegotiation::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
	{
		terminateSession(APresence->streamJid(),AItem.itemJid);
		removeSession(findSession(APresence->streamJid(),AItem.itemJid));
	}
}

void SessionNegotiation::onNotificationActivated(int ANotifyId)
{
	if (FDialogByNotify.contains(ANotifyId))
	{
		IDataDialogWidget *dialog = FDialogByNotify.take(ANotifyId);
		if (dialog)
			WidgetManager::showActivateRaiseWindow(dialog->instance());
		FNotifications->removeNotification(ANotifyId);
	}
}

void SessionNegotiation::onAcceptDialogAccepted()
{
	IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
	if (dialog)
	{
		IStanzaSession &session = dialogSession(dialog);

		if (session.status == IStanzaSession::Init)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session initialization approved by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			session.status = IStanzaSession::Pending;
			IDataForm request = dialog->formWidget()->userDataForm();
			request.title = tr("Session negotiation");
			updateFields(request,session.form,false,false);
			sendSessionData(session,request);
		}
		else if (session.status == IStanzaSession::Accept)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session accept approved by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			if (dialog->formWidget()->dataForm().type == DATAFORM_TYPE_FORM)
			{
				IDataForm submit = FDataForms->dataSubmit(dialog->formWidget()->userDataForm());
				updateFields(submit,session.form,false,false);
				processApply(session,submit);
			}
			else
			{
				IDataForm result = defaultForm(SESSION_FIELD_ACCEPT);
				result.type = DATAFORM_TYPE_RESULT;
				processApply(session,result);
			}
		}
		else if (session.status == IStanzaSession::Renegotiate)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session renegotiation approved by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			IDataForm request = dialog->formWidget()->dataForm();
			if (request.type.isEmpty())
			{
				IDataForm request = dialog->formWidget()->userDataForm();
				request.type = DATAFORM_TYPE_FORM;
				request.title = tr("Session renegotiation");
				sendSessionData(session,request);
			}
			else if (request.type == DATAFORM_TYPE_FORM)
			{
				IDataForm submit = FDataForms->dataSubmit(dialog->formWidget()->userDataForm());
				updateFields(submit,session.form,false,false);
				processApply(session,submit);
			}
			else if (request.type == DATAFORM_TYPE_SUBMIT)
			{
				IDataForm submit = defaultForm(SESSION_FIELD_RENEGOTIATE);
				submit.type = DATAFORM_TYPE_RESULT;
				processApply(session,submit);
			}
		}
	}
}

void SessionNegotiation::onAcceptDialogRejected()
{
	IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
	if (dialog)
	{
		IStanzaSession &session = dialogSession(dialog);
		if (session.status == IStanzaSession::Init)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session initialization rejected by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			session.status = IStanzaSession::Terminate;
			emit sessionTerminated(session);
		}
		else if (session.status == IStanzaSession::Accept)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session accept rejected by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			if (dialog->formWidget()->dataForm().type == DATAFORM_TYPE_FORM)
			{
				session.status = IStanzaSession::Terminate;
				IDataForm submit = FDataForms->dataSubmit(dialog->formWidget()->dataForm());
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
				updateFields(IDataForm(),submit,false,true);
				sendSessionData(session,submit);
			}
			else
			{
				session.status = IStanzaSession::Terminate;
				IDataForm result = defaultForm(SESSION_FIELD_ACCEPT,false);
				result.type = DATAFORM_TYPE_RESULT;
				sendSessionData(session,result);
				emit sessionTerminated(session);
			}
		}
		else if (session.status == IStanzaSession::Renegotiate)
		{
			LOG_STRM_INFO(session.streamJid,QString("Stanza session renegotiation rejected by user, with=%1, sid=%2").arg(session.contactJid.full(),session.sessionId));
			IDataForm request = dialog->formWidget()->dataForm();
			if (request.type.isEmpty())
			{
				terminateSession(session.streamJid,session.contactJid);
			}
			else if (request.type == DATAFORM_TYPE_FORM)
			{
				IDataForm submit = FDataForms->dataSubmit(request);
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
				updateFields(IDataForm(),submit,false,true);
				sendSessionData(session,submit);
			}
			else if (request.type == DATAFORM_TYPE_SUBMIT)
			{
				terminateSession(session.streamJid,session.contactJid);
			}
		}
	}
}

void SessionNegotiation::onAcceptDialogDestroyed(IDataDialogWidget *ADialog)
{
	IStanzaSession &session = dialogSession(ADialog);
	FDialogs[session.streamJid].remove(session.contactJid);
	if (FNotifications)
	{
		int notifyId = FDialogByNotify.key(ADialog);
		FDialogByNotify.remove(notifyId);
		FNotifications->removeNotification(notifyId);
	}
}

void SessionNegotiation::onSessionActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		QString sessionField = action->data(ADR_SESSION_FIELD).toString();
		if (sessionField == SESSION_FIELD_ACCEPT)
			initSession(streamJid,contactJid);
		else if (sessionField == SESSION_FIELD_TERMINATE)
			terminateSession(streamJid,contactJid);
	}
}

void SessionNegotiation::onDiscoInfoRecieved(const IDiscoInfo &AInfo)
{
	foreach(const QString &sessionId, FSuspended.keys())
	{
		IStanzaSession session = findSession(sessionId);
		if (session.status==IStanzaSession::Init && session.contactJid==AInfo.contactJid)
			resumeSession(session.streamJid,session.contactJid);
	}
}
