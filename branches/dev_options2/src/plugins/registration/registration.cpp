#include "registration.h"

#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/dataformtypes.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/internalerrors.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/stanza.h>
#include <utils/logger.h>

#define REGISTRATION_TIMEOUT    30000

#define ADR_StreamJid           Action::DR_StreamJid
#define ADR_ServiceJid          Action::DR_Parametr1
#define ADR_Operation           Action::DR_Parametr2

Registration::Registration()
{
	FDataForms = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
	FPresencePlugin = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
	FXmppUriQueries = NULL;
}

Registration::~Registration()
{

}

void Registration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Registration");
	APluginInfo->description = tr("Allows to register on the Jabber servers and services");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Registration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
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

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
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

	return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool Registration::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_REGISTER_INVALID_FORM,tr("Invalid registration form"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_REGISTER_INVALID_DIALOG,tr("Invalid registration dialog"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_REGISTER_REJECTED_BY_USER,tr("Registration rejected by user"));

	if (FXmppStreams)
	{
		FXmppStreams->registerXmppFeature(XFO_REGISTER,NS_FEATURE_REGISTER);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_REGISTER,this);
	}
	if (FDiscovery)
	{
		registerDiscoFeatures();
		FDiscovery->insertFeatureHandler(NS_JABBER_REGISTER,this,DFO_DEFAULT);
	}
	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_REGISTER);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this,XUHO_DEFAULT);
	}
	return true;
}

bool Registration::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_REGISTER,false);
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

void Registration::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	XmppStanzaError err = AStanza.type()!="result" ? XmppStanzaError(AStanza) : XmppStanzaError::null;

	if (FSendRequests.contains(AStanza.id()))
	{
		QDomElement query = AStanza.firstElement("query",NS_JABBER_REGISTER);

		QDomElement formElem = query.firstChildElement("x");
		while (!formElem.isNull() && (formElem.namespaceURI()!=NS_JABBER_DATA || formElem.attribute("type",DATAFORM_TYPE_FORM)!=DATAFORM_TYPE_FORM))
			formElem = formElem.nextSiblingElement("x");

		if (AStanza.type()=="result" || !formElem.isNull())
		{
			LOG_STRM_INFO(AStreamJid,QString("Registration fields loaded, from=%1, id=%2").arg(AStanza.from(),AStanza.id()));

			IRegisterFields fields;
			fields.fieldMask = 0;
			fields.registered = !query.firstChildElement("registered").isNull();
			fields.serviceJid = AStanza.from();
			fields.instructions = query.firstChildElement("instructions").text();
			if (!query.firstChildElement("username").isNull())
			{
				fields.fieldMask += IRegisterFields::Username;
				fields.username = query.firstChildElement("username").text();
			}
			else if (!query.firstChildElement("name").isNull())
			{
				fields.fieldMask += IRegisterFields::Username;
				fields.username = query.firstChildElement("name").text();
			}
			if (!query.firstChildElement("password").isNull())
			{
				fields.fieldMask += IRegisterFields::Password;
				fields.password = query.firstChildElement("password").text();
			}
			if (!query.firstChildElement("email").isNull())
			{
				fields.fieldMask += IRegisterFields::Email;
				fields.email = query.firstChildElement("email").text();
			}
			fields.key = query.firstChildElement("key").text();

			if (FDataForms)
				fields.form = FDataForms->dataForm(formElem);

			QDomElement oob = query.firstChildElement("x");
			while (!oob.isNull() && oob.namespaceURI()!=NS_JABBER_OOB)
				oob = oob.nextSiblingElement("x");
			if (!oob.isNull())
			{
				fields.fieldMask += IRegisterFields::URL;
				fields.url = oob.firstChildElement("url").text();
			}
			emit registerFields(AStanza.id(),fields);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load registration fields from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
			emit registerError(AStanza.id(),err);
		}
		FSendRequests.removeAll(AStanza.id());
	}
	else if (FSubmitRequests.contains(AStanza.id()))
	{
		if (AStanza.type()=="result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Registration submit accepted, from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
			emit registerSuccessful(AStanza.id());
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Registration submit rejected, from=%1, id=%2: %3").arg(AStanza.from(),AStanza.id(),err.condition()));
			emit registerError(AStanza.id(),err);
		}
		FSubmitRequests.removeAll(AStanza.id());
	}
}

bool Registration::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	Q_UNUSED(AParams);
	if (AAction == "register")
	{
		showRegisterDialog(AStreamJid,AContactJid,IRegistration::Register,NULL);
		return true;
	}
	else if (AAction == "unregister")
	{
		showRegisterDialog(AStreamJid,AContactJid,IRegistration::Unregister,NULL);
		return true;
	}
	return false;
}

bool Registration::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature == NS_JABBER_REGISTER)
	{
		showRegisterDialog(AStreamJid,ADiscoInfo.contactJid,IRegistration::Register,NULL);
		return true;
	}
	return false;
}

Action *Registration::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen() && AFeature==NS_JABBER_REGISTER)
	{
		Menu *regMenu = new Menu(AParent);
		regMenu->setTitle(tr("Registration"));
		regMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_REGISTRATION);

		Action *action = new Action(regMenu);
		action->setText(tr("Register"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_REGISTRATION);
		action->setData(ADR_StreamJid,AStreamJid.full());
		action->setData(ADR_ServiceJid,ADiscoInfo.contactJid.full());
		action->setData(ADR_Operation,IRegistration::Register);
		connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
		regMenu->addAction(action,AG_DEFAULT,false);

		action = new Action(regMenu);
		action->setText(tr("Unregister"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_REGISTRATION_REMOVE);
		action->setData(ADR_StreamJid,AStreamJid.full());
		action->setData(ADR_ServiceJid,ADiscoInfo.contactJid.full());
		action->setData(ADR_Operation,IRegistration::Unregister);
		connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
		regMenu->addAction(action,AG_DEFAULT,false);

		action = new Action(regMenu);
		action->setText(tr("Change password"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_REGISTRATION_CHANGE);
		action->setData(ADR_StreamJid,AStreamJid.full());
		action->setData(ADR_ServiceJid,ADiscoInfo.contactJid.full());
		action->setData(ADR_Operation,IRegistration::ChangePassword);
		connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
		regMenu->addAction(action,AG_DEFAULT,false);

		return regMenu->menuAction();
	}
	return NULL;
}

QList<QString> Registration::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_REGISTER;
}

IXmppFeature *Registration::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_REGISTER)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AXmppStream->streamJid()) : NULL;
		if (account && account->optionsNode().value("register-on-server").toBool())
		{
			LOG_STRM_INFO(AXmppStream->streamJid(),"Registration XMPP stream feature created");
			IXmppFeature *feature = new RegisterStream(FDataForms, AXmppStream);
			connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onXmppFeatureDestroyed()));
			emit featureCreated(feature);
			account->optionsNode().setValue(false,"register-on-server");
			return feature;
		}
	}
	return NULL;
}

QMultiMap<int, IOptionsDialogWidget *> Registration::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (FOptionsManager && nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS)
	{
		widgets.insertMulti(OWO_ACCOUNT_REGISTER, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1)).node("register-on-server"),tr("Register new account on server"),AParent));
	}
	return widgets;
}

IDataFormLocale Registration::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_REGISTER)
	{
		locale.title = tr("Registration Form");
		locale.fields["username"].label = tr("Account Name");
		locale.fields["nick"].label = tr("Nickname");
		locale.fields["password"].label = tr("Password");
		locale.fields["name"].label = tr("Full Name");
		locale.fields["first"].label = tr("Given Name");
		locale.fields["last"].label = tr("Family Name");
		locale.fields["email"].label = tr("Email Address");
		locale.fields["address"].label = tr("Street");
		locale.fields["city"].label = tr("City");
		locale.fields["state"].label = tr("Region");
		locale.fields["zip"].label = tr("Zip Code");
		locale.fields["phone"].label = tr("Telephone Number");
		locale.fields["url"].label = tr("Your Web Page");
	}
	return locale;
}

QString Registration::sendRegisterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	if (FStanzaProcessor && AStreamJid.isValid() && AServiceJid.isValid())
	{
		Stanza request("iq");
		request.setTo(AServiceJid.full()).setType("get").setId(FStanzaProcessor->newId());
		request.addElement("query",NS_JABBER_REGISTER);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,REGISTRATION_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Registration fields request sent, to=%1, id=%2").arg(AStreamJid.full(),request.id()));
			FSendRequests.append(request.id());
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send registration fields request, to=%1, id=%2").arg(AServiceJid.full(),request.id()));
		}
	}
	return QString::null;
}

QString Registration::sendUnregisterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	if (FStanzaProcessor && AStreamJid.isValid() && AServiceJid.isValid())
	{
		Stanza request("iq");
		request.setTo(AServiceJid.full()).setType("set").setId(FStanzaProcessor->newId());
		request.addElement("query",NS_JABBER_REGISTER).appendChild(request.createElement("remove"));
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,REGISTRATION_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Unregistration submit request sent, to=%1, id=%2").arg(AStreamJid.full(),request.id()));
			FSubmitRequests.append(request.id());
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send unregistration submit request, to=%1").arg(AStreamJid.full()));
		}
	}
	return QString::null;
}

QString Registration::sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword)
{
	if (FStanzaProcessor && AStreamJid.isValid() && AServiceJid.isValid())
	{
		Stanza request("iq");
		request.setTo(AServiceJid.full()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = request.addElement("query",NS_JABBER_REGISTER);
		elem.appendChild(request.createElement("username")).appendChild(request.createTextNode(AUserName));
		elem.appendChild(request.createElement("password")).appendChild(request.createTextNode(APassword));
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,REGISTRATION_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Change password registration submit request sent, to=%1, id=%2").arg(AServiceJid.full(),request.id()));
			FSubmitRequests.append(request.id());
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send change password registration submit request, to=%1").arg(AServiceJid.full()));
		}
	}
	return QString::null;
}

QString Registration::sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit)
{
	if (FStanzaProcessor && AStreamJid.isValid())
	{
		Stanza request("iq");
		request.setTo(ASubmit.serviceJid.full()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement query = request.addElement("query",NS_JABBER_REGISTER);
		if (ASubmit.form.type.isEmpty())
		{
			if (!ASubmit.username.isEmpty())
				query.appendChild(request.createElement("username")).appendChild(request.createTextNode(ASubmit.username));
			if (!ASubmit.password.isEmpty())
				query.appendChild(request.createElement("password")).appendChild(request.createTextNode(ASubmit.password));
			if (!ASubmit.email.isEmpty())
				query.appendChild(request.createElement("email")).appendChild(request.createTextNode(ASubmit.email));
			if (!ASubmit.key.isEmpty())
				query.appendChild(request.createElement("key")).appendChild(request.createTextNode(ASubmit.key));
		}
		else if (FDataForms)
		{
			FDataForms->xmlForm(ASubmit.form,query);
		}

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,REGISTRATION_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Registration submit request sent, to=%1, id=%2").arg(ASubmit.serviceJid.full(),request.id()));
			FSubmitRequests.append(request.id());
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send registration submit request, to=%1, id=%2").arg(ASubmit.serviceJid.full(),request.id()));
		}
	}
	return QString::null;
}

bool Registration::showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen())
	{
		RegisterDialog *dialog = new RegisterDialog(this,FDataForms,AStreamJid,AServiceJid,AOperation,AParent);
		connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
		dialog->show();
		return true;
	}
	return false;
}

void Registration::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.active = false;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_REGISTRATION);
	dfeature.var = NS_JABBER_REGISTER;
	dfeature.name = tr("Registration");
	dfeature.description = tr("Supports the registration");
	FDiscovery->insertDiscoFeature(dfeature);
}

void Registration::onRegisterActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_StreamJid).toString();
		Jid serviceJid = action->data(ADR_ServiceJid).toString();
		int operation = action->data(ADR_Operation).toInt();
		showRegisterDialog(streamJid,serviceJid,operation,NULL);
	}
}

void Registration::onXmppFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"Registration XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}

Q_EXPORT_PLUGIN2(plg_registration, Registration)
