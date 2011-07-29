#include "sessionnegotiation.h"

#include <QUuid>
#include <QTextDocument>
#include <QCryptographicHash>

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
	FPresencePlugin = NULL;
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

bool SessionNegotiation::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
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

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
			        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
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
		ushort kindMask = INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AutoActivate;
		ushort kindDefs = INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay;
		FNotifications->registerNotificationType(NNT_SESSION_NEGOTIATION,OWO_NOTIFICATIONS_SESSION_NEGOTIATION,tr("Negotiate session requests"),kindMask,kindDefs);
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
			IStanzaSession &session = FSessions[AStreamJid][contactJid];

			IStanzaSession bareSession = getSession(AStreamJid,contactJid.bare());
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
					if (isAccept && session.status != IStanzaSession::Active)
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

				ErrorHandler err(AStanza.element());
				session.errorCondition = err.condition();

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

				emit sessionTerminated(session);
			}
			else if (session.status == IStanzaSession::Empty)
			{
				removeSession(session);
			}
			AAccept = true;
		}
	}
	return false;
}

bool SessionNegotiation::execDiscoFeature(const Jid &/*AStreamJid*/, const QString &/*AFeature*/, const IDiscoInfo &/*ADiscoInfo*/)
{
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

		IStanzaSession session = getSession(AStreamJid,ADiscoInfo.contactJid);
		if (session.status==IStanzaSession::Empty ||
		    session.status==IStanzaSession::Terminate ||
		    session.status==IStanzaSession::Error)
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

	//MultiSession
	if (ASession.status==IStanzaSession::Init)
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

int SessionNegotiation::sessionAccept(const IStanzaSession &/*ASession*/, const IDataForm &ARequest, IDataForm &ASubmit)
{
	int result = ISessionNegotiator::Skip;

	//Multisession
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

int SessionNegotiation::sessionApply(const IStanzaSession &/*ASession*/)
{
	return ISessionNegotiator::Auto;
}

void SessionNegotiation::sessionLocalize(const IStanzaSession &/*ASession*/, IDataForm &AForm)
{
	//Multi-session
	int index = FDataForms->fieldIndex(SFP_MULTISESSION,AForm.fields);
	if (index>=0)
	{
		AForm.fields[index].label = tr("Allow multiple sessions?");
	}
}

IStanzaSession SessionNegotiation::getSession(const QString &ASessionId) const
{
	foreach (Jid streamJid, FSessions.keys())
		foreach (IStanzaSession session, FSessions.value(streamJid))
			if (session.sessionId == ASessionId)
				return session;
	return IStanzaSession();
}

IStanzaSession SessionNegotiation::getSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FSessions.value(AStreamJid).value(AContactJid);
}

QList<IStanzaSession> SessionNegotiation::getSessions(const Jid &AStreamJid, int AStatus) const
{
	QList<IStanzaSession> sessions;
	foreach(IStanzaSession session, FSessions.value(AStreamJid).values())
		if (session.status == AStatus)
			sessions.append(session);
	return sessions;
}

int SessionNegotiation::initSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	IStanzaSession &session = FSessions[AStreamJid][AContactJid];
	if (AStreamJid != AContactJid &&
	    session.status!=IStanzaSession::Accept &&
	    session.status!=IStanzaSession::Pending &&
	    session.status!=IStanzaSession::Apply &&
	    session.status!=IStanzaSession::Renegotiate &&
	    session.status!=IStanzaSession::Continue)
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
			session.errorCondition.clear();
			session.errorFields.clear();
		}
		else
		{
			session.status = IStanzaSession::Renegotiate;
		}

		int result = 0;
		foreach(ISessionNegotiator *negotiator, FNegotiators)
			result = result | negotiator->sessionInit(session,request);

		if (!isRenegotiate && FDiscovery && !FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_STANZA_SESSION))
		{
			bool infoRequested = !FDiscovery->hasDiscoInfo(AStreamJid,AContactJid) ? FDiscovery->requestDiscoInfo(AStreamJid,AContactJid) : false;
			if (!infoRequested)
			{
				session.status = IStanzaSession::Error;
				session.errorCondition = ErrorHandler::coditionByCode(ErrorHandler::SERVICE_UNAVAILABLE);
				emit sessionTerminated(session);
				return ISessionNegotiator::Cancel;
			}
			else
			{
				session.status = IStanzaSession::Init;
				FSuspended.insert(session.sessionId,IDataForm());
				return ISessionNegotiator::Wait;
			}
		}
		else if ((result & ISessionNegotiator::Cancel) > 0)
		{
			if (!isRenegotiate)
			{
				session.status = IStanzaSession::Terminate;
				emit sessionTerminated(session);
			}
			else
			{
				terminateSession(AStreamJid,AContactJid);
			}
			return ISessionNegotiator::Cancel;
		}
		else if ((result & ISessionNegotiator::Manual) > 0)
		{
			if (!isRenegotiate)
				session.form = clearForm(request);
			localizeSession(session,request);
			showAcceptDialog(session,request);
			return ISessionNegotiator::Manual;
		}
		else if ((result & ISessionNegotiator::Auto) > 0)
		{
			if (!isRenegotiate)
			{
				session.form = clearForm(request);
				session.status = IStanzaSession::Pending;
			}
			else
			{
				FRenegotiate.insert(session.sessionId,request);
			}
			request.type = DATAFORM_TYPE_FORM;
			localizeSession(session,request);
			request.title = isRenegotiate ? tr("Session renegotiation") : tr("Session negotiation");
			sendSessionData(session,request);
			return ISessionNegotiator::Auto;
		}
	}
	return ISessionNegotiator::Skip;
}

void SessionNegotiation::resumeSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FSuspended.contains(FSessions.value(AStreamJid).value(AContactJid).sessionId))
	{
		IStanzaSession &session = FSessions[AStreamJid][AContactJid];
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
}

void SessionNegotiation::terminateSession(const Jid &AStreamJid, const Jid &AContactJid)
{
	IStanzaSession &session = FSessions[AStreamJid][AContactJid];
	if (session.status != IStanzaSession::Empty &&
	    session.status != IStanzaSession::Init &&
	    session.status != IStanzaSession::Terminate &&
	    session.status != IStanzaSession::Error)
	{
		IDataForm request = defaultForm(SESSION_FIELD_TERMINATE);
		request.type = DATAFORM_TYPE_SUBMIT;
		session.status = IStanzaSession::Terminate;
		sendSessionData(session,request);
		emit sessionTerminated(session);
	}
}

void SessionNegotiation::showSessionParams(const Jid &AStreamJid, const Jid &AContactJid)
{
	IStanzaSession session = getSession(AStreamJid,AContactJid);
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
		FNegotiators.insert(AOrder,ANegotiator);
}

void SessionNegotiation::removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder)
{
	if (FNegotiators.contains(AOrder,ANegotiator))
		FNegotiators.remove(AOrder,ANegotiator);
}

bool SessionNegotiation::sendSessionData(const IStanzaSession &ASession, const IDataForm &AForm) const
{
	if (FStanzaProcessor && FDataForms && !AForm.fields.isEmpty())
	{
		Stanza data("message");
		data.setType("normal").setTo(ASession.contactJid.eFull());
		data.addElement("thread").appendChild(data.createTextNode(ASession.sessionId));
		QDomElement featureElem = data.addElement("feature",NS_FEATURENEG);
		IDataForm form = AForm;
		form.pages.clear();
		FDataForms->xmlForm(form,featureElem);
		return FStanzaProcessor->sendStanzaOut(ASession.streamJid,data);
	}
	return false;
}

bool SessionNegotiation::sendSessionError(const IStanzaSession &ASession, const IDataForm &ARequest) const
{
	if (FStanzaProcessor && FDataForms && !ASession.errorCondition.isEmpty())
	{
		Stanza data("message");
		data.setType("error").setTo(ASession.contactJid.eFull());
		data.addElement("thread").appendChild(data.createTextNode(ASession.sessionId));
		QDomElement featureElem = data.addElement("feature",NS_FEATURENEG);
		IDataForm request = ARequest;
		request.pages.clear();
		FDataForms->xmlForm(request,featureElem);
		QDomElement errorElem = data.addElement("error");
		errorElem.setAttribute("code",ErrorHandler::codeByCondition(ASession.errorCondition));
		errorElem.setAttribute("type",ErrorHandler::typeToString(ErrorHandler::typeByCondition(ASession.errorCondition)));
		errorElem.appendChild(data.createElement(ASession.errorCondition,EHN_DEFAULT));
		if (!ASession.errorFields.isEmpty())
		{
			QDomElement featureElem = errorElem.appendChild(data.createElement("feature",NS_FEATURENEG)).toElement();
			foreach(QString var, ASession.errorFields) {
				featureElem.appendChild(data.createElement("field")).toElement().setAttribute("var",var); }
		}
		return FStanzaProcessor->sendStanzaOut(ASession.streamJid,data);
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
			ASession.status = IStanzaSession::Error;
			ASession.errorCondition = ErrorHandler::coditionByCode(ErrorHandler::FEATURE_NOT_IMPLEMENTED);
			ASession.errorFields = unsubmitedFields(ARequest,submit,true);
			sendSessionError(ASession,ARequest);
		}
		else if ((result & ISessionNegotiator::Cancel) > 0)
		{
			ASession.status = IStanzaSession::Terminate;
			submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
			updateFields(IDataForm(),submit,false,true);
			sendSessionData(ASession,submit);
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			FSuspended.insert(ASession.sessionId,ARequest);
		}
		else if ((result & ISessionNegotiator::Manual) > 0)
		{
			updateFields(submit,ASession.form,false,true);
			IDataForm request = ASession.form;
			request.pages = submit.pages;
			localizeSession(ASession,request);
			showAcceptDialog(ASession,request);
		}
		else
		{
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
				ASession.status = IStanzaSession::Error;
				ASession.errorCondition = ErrorHandler::coditionByCode(ErrorHandler::NOT_ACCEPTABLE);
				ASession.errorFields = unsubmitedFields(ARequest,submit,true);
				sendSessionError(ASession,ARequest);
				emit sessionTerminated(ASession);
			}
			else if ((result & ISessionNegotiator::Cancel) > 0)
			{
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
				FSuspended.insert(ASession.sessionId,ARequest);
			}
			else if ((result & ISessionNegotiator::Manual) > 0)
			{
				updateFields(ARequest,ASession.form,false,false);
				IDataForm request = ASession.form;
				request.pages = submit.pages;
				localizeSession(ASession,request);
				request = FDataForms->dataShowSubmit(request,ARequest);
				showAcceptDialog(ASession,request);
			}
			else
			{
				updateFields(ARequest,ASession.form,false,false);
				processApply(ASession,submit);
			}
		}
		else
		{
			ASession.status = IStanzaSession::Terminate;
			updateFields(ARequest,ASession.form,true,false);
			emit sessionTerminated(ASession);
		}
	}
	else if (ARequest.type == DATAFORM_TYPE_RESULT)
	{
		if (FDataForms->fieldValue(SESSION_FIELD_ACCEPT,ARequest.fields).toBool())
		{
			ASession.status = IStanzaSession::Active;
			emit sessionActivated(ASession);
		}
		else
		{
			ASession.status = IStanzaSession::Terminate;
			updateFields(ARequest,ASession.form,true,false);
			emit sessionTerminated(ASession);
		}
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
				ASession.status = IStanzaSession::Terminate;
				IDataForm submit = ASubmit;
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
				sendSessionData(ASession,submit);
				emit sessionTerminated(ASession);
			}
			else if (ASubmit.type == DATAFORM_TYPE_SUBMIT)
			{
				ASession.status = IStanzaSession::Active;
				IDataForm submit = ASubmit;
				submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
				sendSessionData(ASession,submit);
			}
			else
			{
				terminateSession(ASession.streamJid,ASession.contactJid);
			}
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			FSuspended.insert(ASession.sessionId,ASubmit);
		}
		else
		{
			if (isAccept)
			{
				ASession.status = ASubmit.type==DATAFORM_TYPE_RESULT ? IStanzaSession::Active : IStanzaSession::Pending;
				sendSessionData(ASession,ASubmit);
				if (ASession.status == IStanzaSession::Active)
					emit sessionActivated(ASession);
			}
			else
			{
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

		if (!FDataForms->isSubmitValid(ARequest,submit) || (result & ISessionNegotiator::Cancel) > 0)
		{
			ASession.status = IStanzaSession::Active;
			submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
			sendSessionData(ASession,submit);
		}
		else if ((result & ISessionNegotiator::Wait) > 0)
		{
			FSuspended.insert(ASession.sessionId,ARequest);
		}
		else if ((result & ISessionNegotiator::Manual) > 0)
		{
			IDataForm request = ARequest;
			request.pages = submit.pages;
			updateFields(submit,request,false,false);
			localizeSession(ASession,request);
			showAcceptDialog(ASession,request);
		}
		else
		{
			updateFields(submit,ASession.form,false,false);
			processApply(ASession,submit);
		}
	}
	else if (ARequest.type == DATAFORM_TYPE_SUBMIT && FRenegotiate.contains(ASession.sessionId))
	{
		ASession.status = IStanzaSession::Renegotiate;

		IDataForm form = FRenegotiate.take(ASession.sessionId);
		bool accepted = FDataForms->fieldValue(SESSION_FIELD_RENEGOTIATE,ARequest.fields).toBool();
		if (accepted && FDataForms->isSubmitValid(form,ARequest))
		{

			IDataForm submit = defaultForm(SESSION_FIELD_RENEGOTIATE);
			submit.type = DATAFORM_TYPE_RESULT;

			updateFields(ARequest,ASession.form,false,false);
			processApply(ASession,submit);
		}
		else
		{
			terminateSession(ASession.streamJid,ASession.contactJid);
		}
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
				ASession.status = IStanzaSession::Error;
				ASession.errorCondition = ErrorHandler::coditionByCode(ErrorHandler::NOT_ACCEPTABLE);
				sendSessionError(ASession,ARequest);
			}
			else if ((result & ISessionNegotiator::Wait) > 0)
			{
				FSuspended.insert(ASession.sessionId,ARequest);
			}
			else
			{
				IDataForm submit = defaultForm(SESSION_FIELD_CONTINUE,resource);
				submit.type = DATAFORM_TYPE_RESULT;
				sendSessionData(ASession,submit);
				ASession.status = IStanzaSession::Active;
				ASession.contactJid.setResource(resource);
				emit sessionActivated(ASession);
			}
		}
	}
}

void SessionNegotiation::processTerminate(IStanzaSession &ASession, const IDataForm &ARequest)
{
	if (ARequest.type == DATAFORM_TYPE_SUBMIT)
	{
		ASession.status = IStanzaSession::Terminate;
		emit sessionTerminated(ASession);
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
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog->instance(),MNI_SNEGOTIATION,0,0,"windowIcon");
			dialog->dialogButtons()->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
			connect(dialog->instance(),SIGNAL(accepted()),SLOT(onAcceptDialogAccepted()));
			connect(dialog->instance(),SIGNAL(rejected()),SLOT(onAcceptDialogRejected()));
			connect(dialog->instance(),SIGNAL(dialogDestroyed(IDataDialogWidget *)),SLOT(onAcceptDialogDestroyed(IDataDialogWidget *)));
			FDialogs[ASession.streamJid].insert(ASession.contactJid,dialog);
		}
		else
			dialog->setForm(ARequest);

		if (FNotifications && !dialog->instance()->isVisible())
		{
			INotification notify;
			notify.kinds = FNotifications->notificationKinds(NNT_SESSION_NEGOTIATION);
			if (notify.kinds > 0)
			{
				notify.type = NNT_SESSION_NEGOTIATION;
				notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SNEGOTIATION));
				notify.data.insert(NDR_TOOLTIP,tr("Session negotiation - %1").arg(ASession.contactJid.full()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Session negotiation"));
				notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(ASession.streamJid,ASession.contactJid));
				notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(ASession.contactJid));
				notify.data.insert(NDR_POPUP_HTML, Qt::escape(notify.data.value(NDR_TOOLTIP).toString()));
				notify.data.insert(NDR_SOUND_FILE, SDF_SNEGOTIATION_REQUEST);
				int notifyId = FNotifications->appendNotification(notify);
				FDialogByNotify.insert(notifyId,dialog);
			}
		}
		else
			dialog->instance()->show();
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
	AForm.title = tr("Session negotiation - %1").arg(ASession.contactJid.full());
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
		foreach(IDataField field, ASourse.fields)
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
	foreach(IDataField field, AForm.fields)
	{
		IDataField clearField;
		clearField.type = field.type;
		clearField.var = field.var;
		clearField.value = field.value;
		clearField.required = field.required;
		foreach (IDataOption option, field.options)
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
	foreach(IDataField rField, ARequest.fields)
	{
		int index = FDataForms->fieldIndex(rField.var,ASubmit.fields);
		IDataField sField = index>=0 ? ASubmit.fields.at(index) : IDataField();
		if ((rField.required || !ARequired) && FDataForms->isFieldEmpty(ASubmit.fields.at(index)))
			fields.append(rField.var);
	}
	return fields;
}

IStanzaSession &SessionNegotiation::dialogSession(IDataDialogWidget *ADialog)
{
	foreach(Jid streamJid, FDialogs.keys())
		if (FDialogs.value(streamJid).values().contains(ADialog))
			return FSessions[streamJid][FDialogs.value(streamJid).key(ADialog)];
	return FSessions[""][""];
}

void SessionNegotiation::onStreamOpened(IXmppStream *AXmppStream)
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
}

void SessionNegotiation::onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem)
{
	if (AItem.show == IPresence::Offline || AItem.show == IPresence::Error)
	{
		terminateSession(APresence->streamJid(),AItem.itemJid);
		removeSession(getSession(APresence->streamJid(),AItem.itemJid));
	}
}

void SessionNegotiation::onStreamAboutToClose(IXmppStream *AXmppStream)
{
	QList<IStanzaSession> sessions = FSessions.value(AXmppStream->streamJid()).values();
	foreach(IStanzaSession session, sessions)
	{
		terminateSession(session.streamJid,session.contactJid);
		removeSession(session);
	}
}

void SessionNegotiation::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor && FDataForms)
	{
		FStanzaProcessor->removeStanzaHandle(FSHISession.take(AXmppStream->streamJid()));
	}
	FDialogs.remove(AXmppStream->streamJid());
	FSessions.remove(AXmppStream->streamJid());
}

void SessionNegotiation::onNotificationActivated(int ANotifyId)
{
	if (FDialogByNotify.contains(ANotifyId))
	{
		IDataDialogWidget *dialog = FDialogByNotify.take(ANotifyId);
		if (dialog)
			dialog->instance()->show();
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
			session.status = IStanzaSession::Pending;
			IDataForm request = dialog->formWidget()->userDataForm();
			request.title = tr("Session negotiation");
			updateFields(request,session.form,false,false);
			sendSessionData(session,request);
		}
		else if (session.status == IStanzaSession::Accept)
		{
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
			session.status = IStanzaSession::Terminate;
			emit sessionTerminated(session);
		}
		else if (session.status == IStanzaSession::Accept)
		{
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
	foreach(QString sessionId, FSuspended.keys())
	{
		IStanzaSession session = getSession(sessionId);
		if (session.status==IStanzaSession::Init && session.contactJid==AInfo.contactJid)
			resumeSession(session.streamJid,session.contactJid);
	}
}

Q_EXPORT_PLUGIN2(plg_sessionnegotiation, SessionNegotiation)
