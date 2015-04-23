#include "createaccountwizard.h"

#include <QVariant>
#include <QLineEdit>
#include <QCompleter>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSignalMapper>
#include <QTextDocument>
#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <definitions/statisticsparams.h>
#include <utils/logger.h>
#include <utils/options.h>
#include <utils/pluginhelper.h>

#define WF_WIZARD_MODE                 "WizardMode"
#define WF_APPEND_SERVICE              "AppendService"
#define WF_APPEND_NODE                 "AppendNode"
#define WF_APPEND_DOMAIN               "AppendDomain"
#define WF_APPEND_PASSWORD             "AppendPassword"
#define WF_APPEND_SAVE_PASSWORD        "AppendSavePassword"
#define WF_APPEND_CONN_ENGINE          "AppendConnectionEngine"
#define WF_APPEND_SHOW_SETTINGS        "AppendShowSettings"
#define WF_REGISTER_ID                 "RegisterId"
#define WF_REGISTER_NODE               "RegisterNode"
#define WF_REGISTER_DOMAIN             "RegisterDomain"
#define WF_REGISTER_PASSWORD           "RegisterPassword"
#define WF_REGISTER_SAVE_PASSWORD      "RegisterSavePassword"
#define WF_REGISTER_CONN_ENGINE        "RegisterConnectionEngine"
#define WF_REGISTER_SHOW_SETTINGS      "RegisterShowSerrings"

#define ACCOUNT_CONNECTION_OPTIONS     Options::node(OPV_ACCOUNT_CONNECTION_ITEM,"CreateAccountWizard")

ConnectionOptionsWidget::ConnectionOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	FConnectionEngine = NULL;
	lblConnectionSettings = NULL;
	odwConnectionSettings = NULL;

	IConnectionManager *connManager = PluginHelper::pluginInstance<IConnectionManager>();
	if (connManager)
	{
		QString engineId = Options::defaultValue(OPV_ACCOUNT_CONNECTION_TYPE).toString();
		engineId = !connManager->connectionEngines().contains(engineId) ? connManager->connectionEngines().value(0) : engineId;
		FConnectionEngine = connManager->findConnectionEngine(engineId);
		if (FConnectionEngine)
		{
			odwConnectionSettings = FConnectionEngine->connectionSettingsWidget(ACCOUNT_CONNECTION_OPTIONS,this);
			if (odwConnectionSettings)
			{
				QVBoxLayout *layout = new QVBoxLayout(this);
				layout->setMargin(0);

				lblConnectionSettings = new QLabel(this);
				onConnectionSettingsLinkActivated("hide");
				connect(lblConnectionSettings,SIGNAL(linkActivated(QString)),SLOT(onConnectionSettingsLinkActivated(QString)));
				layout->addWidget(lblConnectionSettings);

				odwConnectionSettings->instance()->setVisible(false);
				layout->addWidget(odwConnectionSettings->instance());
			}
		}
	}
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{
	OptionsNode node = ACCOUNT_CONNECTION_OPTIONS;
	node.parent().removeNode(node.name(), node.nspace());
}

void ConnectionOptionsWidget::applyOptions() const
{
	if (odwConnectionSettings)
		odwConnectionSettings->apply();
}

void ConnectionOptionsWidget::saveOptions(IAccount *AAccount) const
{
	if (FConnectionEngine!=NULL && odwConnectionSettings!=NULL)
	{
		AAccount->optionsNode().setValue(FConnectionEngine->engineId(),"connection-type");
		FConnectionEngine->saveConnectionSettings(odwConnectionSettings, AAccount->optionsNode().node("connection",FConnectionEngine->engineId()));
	}
}

QString ConnectionOptionsWidget::connectionEngine() const
{
	return FConnectionEngine!=NULL ? FConnectionEngine->engineId() : QString::null;
}

void ConnectionOptionsWidget::setConnectionEngine(const QString &AEngineId)
{
	Q_UNUSED(AEngineId);
}

void ConnectionOptionsWidget::onConnectionSettingsLinkActivated(const QString &ALink)
{
	if (ALink == "hide")
	{
		odwConnectionSettings->instance()->setVisible(false);
		lblConnectionSettings->setText(QString("<a href='show'>%1</a>").arg(tr("Show connection settings")));
	}
	else if (ALink == "show")
	{
		odwConnectionSettings->instance()->setVisible(true);
		lblConnectionSettings->setText(QString("<a href='hide'>%1</a>").arg(tr("Hide connection settings")));
	}
}


CreateAccountWizard::CreateAccountWizard(QWidget *AParent) : QWizard(AParent)
{
	REPORT_VIEW;
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Create Account Wizard"));
#ifndef Q_WS_MAC
	setWizardStyle(QWizard::ModernStyle);
#endif

	setPage(PageWizardStart, new WizardStartPage(this));
	
	setPage(PageAppendService, new AppendServicePage(this));
	setPage(PageAppendSettings, new AppendSettingsPage(this));
	setPage(PageAppendCheck, new AppendCheckPage(this));

	setPage(PageRegisterServer, new	RegisterServerPage(this));
	setPage(PageRegisterRequest, new	RegisterRequestPage(this));
	setPage(PageRegisterSubmit, new	RegisterSubmitPage(this));

	setStartId(PageWizardStart);
}

void CreateAccountWizard::accept()
{
	Jid streamJid;
	if (field(WF_WIZARD_MODE).toInt() == ModeAppend)
	{
		streamJid.setNode(field(WF_APPEND_NODE).toString());
		streamJid.setDomain(field(WF_APPEND_DOMAIN).toString());
	}
	else if (field(WF_WIZARD_MODE).toInt() == ModeRegister)
	{
		streamJid.setNode(field(WF_REGISTER_NODE).toString());
		streamJid.setDomain(field(WF_REGISTER_DOMAIN).toString());
	}
	LOG_INFO(QString("Creating account: jid=%1").arg(streamJid.full()));

	IAccountManager *accountManager = PluginHelper::pluginInstance<IAccountManager>();
	IAccount *account = accountManager!=NULL ? accountManager->createAccount(streamJid,streamJid.uBare()) : NULL;
	if (account != NULL)
	{
		REPORT_EVENT(SEVP_ACCOUNT_CREATED,1);

		bool showSettings = false;
		if (field(WF_WIZARD_MODE).toInt() == ModeAppend)
		{
			AppendSettingsPage *settingsPage = qobject_cast<AppendSettingsPage *>(page(PageAppendSettings));
			if (settingsPage != NULL)
				settingsPage->saveAccountSettings(account);
			showSettings = field(WF_APPEND_SHOW_SETTINGS).toBool();
		}
		else if (field(WF_WIZARD_MODE).toInt() == ModeRegister)
		{
			RegisterServerPage *serverPage = qobject_cast<RegisterServerPage *>(page(PageRegisterServer));
			if (serverPage != NULL)
				serverPage->saveAccountSettings(account);
			showSettings = field(WF_REGISTER_SHOW_SETTINGS).toBool();
		}

		if (showSettings)
		{
			IOptionsManager *optionsManager = PluginHelper::pluginInstance<IOptionsManager>();
			if (optionsManager != NULL)
			{
				QString rootId = OPN_ACCOUNTS"."+account->accountId().toString();
				optionsManager->showOptionsDialog(QString::null, rootId, parentWidget());
			}
		}

		account->setActive(true);
		QWizard::accept();
	}
	else
	{
		QMessageBox::critical(this,tr("Account not Created"), tr("Failed to create account %1 due to internal error.").arg(streamJid.uBare()));
		REPORT_ERROR("Failed to create account: Account not created");
	}
}

WizardStartPage::WizardStartPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Add Jabber/XMPP Account"));
	setSubTitle(tr("This wizard will help you to add an existing account or register a new one"));

	rbtAppendAccount = new QRadioButton(this);
	rbtAppendAccount->setText(tr("I want to add my existing account"));

	rbtRegisterAccount = new QRadioButton(this);
	rbtRegisterAccount->setText(tr("I want to register a new account"));

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(rbtAppendAccount);
	layout->addWidget(rbtRegisterAccount);
	layout->setSpacing(layout->spacing()*2);

	setWizardMode(CreateAccountWizard::ModeAppend);
	registerField(WF_WIZARD_MODE,this,"wizardMode");
}

int WizardStartPage::nextId() const
{
	switch (field(WF_WIZARD_MODE).toInt())
	{
	case CreateAccountWizard::ModeAppend:
		return CreateAccountWizard::PageAppendService;
	case CreateAccountWizard::ModeRegister:
		return CreateAccountWizard::PageRegisterServer;
	}
	return -1;
}

int WizardStartPage::wizardMode() const
{
	if (rbtAppendAccount->isChecked())
		return CreateAccountWizard::ModeAppend;
	else if (rbtRegisterAccount->isChecked())
		return CreateAccountWizard::ModeRegister;
	return -1;
}

void WizardStartPage::setWizardMode(int AMode)
{
	if (AMode == CreateAccountWizard::ModeAppend)
		rbtAppendAccount->setChecked(true);
	else if (AMode == CreateAccountWizard::ModeRegister)
		rbtRegisterAccount->setChecked(true);
}

AppendServicePage::AppendServicePage(QWidget *AParent) : QWizardPage(AParent)
{
	const struct { int type; QString name; } services[CreateAccountWizard::Service_Count] = {
		{ CreateAccountWizard::ServiceJabber,        tr("Jabber/XMPP")        },
		{ CreateAccountWizard::ServiceGoogle,        tr("Google Talk")        },
		{ CreateAccountWizard::ServiceYandex,        tr("Yandex Online")      },
		{ CreateAccountWizard::ServiceOdnoklassniki, tr("Odnoklassniki")      },
		{ CreateAccountWizard::ServiceLiveJournal,   tr("LiveJournal")        },
		{ CreateAccountWizard::ServiceQIP,           tr("QIP")                },
	};

	setTitle(tr("Select Service"));
	setSubTitle(tr("Select the service for which you already have a registered account"));

	QSignalMapper *signalMapper = new QSignalMapper(this);
	connect(signalMapper,SIGNAL(mapped(int)),SLOT(onServiceButtonClicked(int)));

	QVBoxLayout *layout = new QVBoxLayout(this);
	for (int i=0; i<CreateAccountWizard::Service_Count; i++)
	{
		QRadioButton *button = new QRadioButton(this);
		button->setText(services[i].name);
		connect(button,SIGNAL(clicked()),signalMapper,SLOT(map()));

		signalMapper->setMapping(button, services[i].type);
		FTypeButton.insert(services[i].type, button);

		layout->addWidget(button);
	}
	layout->setSpacing(layout->spacing()*2);
	
	setServiceType(CreateAccountWizard::ServiceJabber);
	registerField(WF_APPEND_SERVICE,this,"serviceType");
}

int AppendServicePage::serviceType() const
{
	return FServiceType;
}

void AppendServicePage::setServiceType(int AType)
{
	FServiceType = AType;
	FTypeButton.value(AType)->setChecked(true);
}

void AppendServicePage::onServiceButtonClicked(int AType)
{
	setField(WF_APPEND_SERVICE,AType);
}


AppendSettingsPage::AppendSettingsPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Account Settings"));
	setSubTitle(tr("Fill out the account credentials and connection settings"));

	QLabel *lblJabberId = new QLabel(this);
	lblJabberId->setText(tr("Jabber ID:"));
	
	QLineEdit *lneNode = new QLineEdit(this);
	registerField(WF_APPEND_NODE"*",lneNode);
	
	QLabel *lblDog = new QLabel("@", this);
	lblDog->setText("@");
	
	cmbDomain = new QComboBox(this);
	registerField(WF_APPEND_DOMAIN"*",this,"accountDomain");
	
	QLabel *lblPassword = new QLabel(this);
	lblPassword->setText(tr("Password:"));

	QLineEdit *lnePassword = new QLineEdit(this);
	lnePassword->setEchoMode(QLineEdit::Password);
	registerField(WF_APPEND_PASSWORD"*",lnePassword);

	QCheckBox *chbSavePassword = new QCheckBox(this);
	chbSavePassword->setChecked(true);
	chbSavePassword->setText(tr("Save password"));
	registerField(WF_APPEND_SAVE_PASSWORD,chbSavePassword);

	QGridLayout *gltAccount = new QGridLayout;
	gltAccount->addWidget(lblJabberId,0,0);
	gltAccount->addWidget(lneNode,0,1);
	gltAccount->addWidget(lblDog,0,2);
	gltAccount->addWidget(cmbDomain,0,3);
	gltAccount->addWidget(lblPassword,1,0);
	gltAccount->addWidget(lnePassword,1,1);
	gltAccount->addWidget(chbSavePassword,1,3);
	gltAccount->setColumnStretch(1,1);
	gltAccount->setColumnStretch(3,1);

	cowConnOptions = new ConnectionOptionsWidget(this);
	registerField(WF_APPEND_CONN_ENGINE,cowConnOptions,"connectionEngine");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addLayout(gltAccount);
	layout->addSpacing(20);
	layout->addWidget(cowConnOptions);
	layout->setSpacing(layout->spacing()*2);
}

void AppendSettingsPage::initializePage()
{
	QWizardPage::initializePage();

	cmbDomain->clear();
	switch (field(WF_APPEND_SERVICE).toInt())
	{
	case CreateAccountWizard::ServiceJabber:
		{
			cmbDomain->setEditable(true);
			connect(cmbDomain->lineEdit(),SIGNAL(textChanged(QString)),SIGNAL(completeChanged()));
		}
		break;
	case CreateAccountWizard::ServiceGoogle:
		{
			static const QStringList domains = QStringList() 
				<< "gmail.com" << "googlemail.com";

			cmbDomain->setEditable(false);
			cmbDomain->addItems(domains);
		}
		break;
	case CreateAccountWizard::ServiceYandex:
		{
			static const QStringList domains = QStringList() 
				<< "ya.ru" << "yandex.ru" << "yandex.net" << "yandex.com" << "yandex.by"
				<< "yandex.kz" << "yandex.ua" << "yandex-co.ru" << "narod.ru";

			cmbDomain->setEditable(false);
			cmbDomain->addItems(domains);
		}
		break;
	case CreateAccountWizard::ServiceOdnoklassniki:
		{
			static const QStringList domains = QStringList() 
				<< "odnoklassniki.ru";

			cmbDomain->setEditable(false);
			cmbDomain->addItems(domains);
		}
		break;
	case CreateAccountWizard::ServiceLiveJournal:
		{
			static const QStringList domains = QStringList()
				<< "livejournal.com";

			cmbDomain->setEditable(false);
			cmbDomain->addItems(domains);
		}
		break;
	case CreateAccountWizard::ServiceQIP:
		{
			static const QStringList domains = QStringList()
				<< "qip.ru" << "pochta.ru" << "fromru.com" << "front.ru" << "hotbox.ru"
				<< "hotmail.ru"	<< "krovatka.su" << "land.ru"	<< "mail15.com"	<< "mail333.com"
				<< "newmail.ru"	<< "nightmail.ru"	<< "nm.ru"	<< "pisem.net"	<< "pochtamt.ru"
				<< "pop3.ru"	<< "rbcmail.ru"	<< "smtp.ru"	<< "5ballov.ru"	<< "aeterna.ru"
				<< "ziza.ru"	<< "memori.ru"	<< "photofile.ru"	<< "fotoplenka.ru";

			cmbDomain->setEditable(false);
			cmbDomain->addItems(domains);
		}
		break;
	}
}

bool AppendSettingsPage::isComplete() const
{
	Jid streamJid(field(WF_APPEND_NODE).toString(), field(WF_APPEND_DOMAIN).toString(), QString::null);
	if (!streamJid.isValid() || streamJid.node().isEmpty())
		return false;
	return QWizardPage::isComplete();
}

bool AppendSettingsPage::validatePage()
{
	Jid streamJid(field(WF_APPEND_NODE).toString(), field(WF_APPEND_DOMAIN).toString(), QString::null);

	IAccountManager *accountManager = PluginHelper::pluginInstance<IAccountManager>();
	if (accountManager!=NULL && accountManager->findAccountByStream(streamJid)!=NULL)
	{
		QMessageBox::warning(this,tr("Duplicate Account"),tr("Account with Jabber ID <b>%1</b> already exists.").arg(streamJid.uBare().toHtmlEscaped()));
		return false;
	}

	cowConnOptions->applyOptions();

	return QWizardPage::validatePage();
}

QString AppendSettingsPage::accountDomain() const
{
	if (cmbDomain->isEditable())
		return cmbDomain->lineEdit()->text();
	else
		return cmbDomain->currentText();
}

void AppendSettingsPage::setAccountDomain( const QString &ADomain )
{
	if (cmbDomain->isEditable())
		cmbDomain->lineEdit()->setText(ADomain);
	else
		cmbDomain->setCurrentIndex(cmbDomain->findText(ADomain));
	emit completeChanged();
}

void AppendSettingsPage::saveAccountSettings(IAccount *AAccount) const
{
	if (field(WF_APPEND_SAVE_PASSWORD).toBool())
		AAccount->setPassword(field(WF_APPEND_PASSWORD).toString());
	cowConnOptions->saveOptions(AAccount);
}


AppendCheckPage::AppendCheckPage(QWidget *AParent) : QWizardPage(AParent)
{
	setFinalPage(true);
	setTitle(tr("Connection to Server"));
	setSubTitle(tr("Wizard checks possibility to connect with the specified credentials"));

	lblCaption = new QLabel(this);
	lblCaption->setAlignment(Qt::AlignCenter);

	prbProgress = new QProgressBar(this);
	prbProgress->setRange(0,0);
	prbProgress->setTextVisible(false);
	prbProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	
	lblError = new QLabel(this);
	lblError->setWordWrap(true);
	lblError->setAlignment(Qt::AlignCenter);
	
	lblDescription = new QLabel(this);
	lblDescription->setWordWrap(true);
	lblDescription->setAlignment(Qt::AlignCenter);

	chbShowSettings = new QCheckBox(this);
	chbShowSettings->setText(tr("Show account settings window"));
	registerField(WF_APPEND_SHOW_SETTINGS,chbShowSettings);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addStretch();
	layout->addWidget(lblCaption);
	layout->addWidget(prbProgress);
	layout->addWidget(lblError);
	layout->addWidget(lblDescription);
	layout->addStretch();
	layout->addWidget(chbShowSettings);
	layout->setSpacing(layout->spacing()*2);

	FXmppStream = NULL;
	FConnecting = false;
}

AppendCheckPage::~AppendCheckPage()
{
	if (FXmppStream)
		delete FXmppStream->instance();
}

void AppendCheckPage::initializePage()
{
	QWizardPage::initializePage();

	if (FXmppStream != NULL)
	{
		IConnection *conn = FXmppStream->connection();
		conn->engine()->loadConnectionSettings(conn,ACCOUNT_CONNECTION_OPTIONS);
	}
	else
	{
		FXmppStream = createXmppStream();
	}

	if (FXmppStream)
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Connecting...")));
		
		lblError->setVisible(false);
		prbProgress->setVisible(true);
		lblDescription->setVisible(false);
		chbShowSettings->setVisible(false);

		Jid streamJid(field(WF_APPEND_NODE).toString(), field(WF_APPEND_DOMAIN).toString(), "Wizard");
		FXmppStream->setStreamJid(streamJid);
		FXmppStream->setPassword(field(WF_APPEND_PASSWORD).toString());
	}
	
	if (FXmppStream==NULL || !FXmppStream->open())
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Failed to check connection :(")));
		lblError->setText(tr("Internal Error"));
		lblDescription->setText(tr("Click 'Back' button to change the account credentials or the 'Finish' button to add the account as is."));

		lblError->setVisible(true);
		prbProgress->setVisible(false);
		lblDescription->setVisible(true);
		chbShowSettings->setVisible(true);
	}
	else
	{
		FConnecting = true;
	}

	emit completeChanged();
}

void AppendCheckPage::cleanupPage()
{
	if (FXmppStream)
		FXmppStream->abort(XmppError::null);
	QWizardPage::cleanupPage();
}

bool AppendCheckPage::isComplete() const
{
	if (FConnecting)
		return false;
	return QWizardPage::isComplete();
}

int AppendCheckPage::nextId() const
{
	return -1;
}

IXmppStream *AppendCheckPage::createXmppStream() const
{
	IXmppStreamManager *xmppStreamManager = PluginHelper::pluginInstance<IXmppStreamManager>();
	IConnectionManager *connManager = PluginHelper::pluginInstance<IConnectionManager>();
	IConnectionEngine *connEngine = connManager!=NULL ? connManager->findConnectionEngine(field(WF_APPEND_CONN_ENGINE).toString()) : NULL;
	if (xmppStreamManager!=NULL && connManager!=NULL && connEngine!=NULL)
	{
		Jid streamJid(field(WF_APPEND_NODE).toString(), field(WF_APPEND_DOMAIN).toString(), QString::null);

		IXmppStream *xmppStream = xmppStreamManager->createXmppStream(streamJid);
		xmppStream->setEncryptionRequired(true);
		connect(xmppStream->instance(),SIGNAL(opened()),SLOT(onXmppStreamOpened()));
		connect(xmppStream->instance(),SIGNAL(error(const XmppError &)),SLOT(onXmppStreamError(const XmppError &)));

		IConnection *conn = connEngine->newConnection(ACCOUNT_CONNECTION_OPTIONS,xmppStream->instance());
		xmppStream->setConnection(conn);

		return xmppStream;
	}
	return NULL;
}

void AppendCheckPage::onXmppStreamOpened()
{
	lblCaption->setText(QString("<h2>%1</h2>").arg(tr("You have successfully connected!")));
	lblDescription->setText(tr("Account credentials successfully checked, click 'Finish' button to add the account."));

	lblError->setVisible(false);
	prbProgress->setVisible(false);
	lblDescription->setVisible(true);
	chbShowSettings->setVisible(true);

	FConnecting = false;
	FXmppStream->close();

	emit completeChanged();
}

void AppendCheckPage::onXmppStreamError(const XmppError &AError)
{
	lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Failed to connect :(")));
	lblError->setText(AError.errorMessage());
	lblDescription->setText(tr("Click 'Back' button to change the account credentials or the 'Finish' button to add the account as is."));

	lblError->setVisible(true);
	prbProgress->setVisible(false);
	lblDescription->setVisible(true);
	chbShowSettings->setVisible(true);

	FConnecting = false;
	FXmppStream->close();

	emit completeChanged();
}


RegisterServerPage::RegisterServerPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Select Server"));
	setSubTitle(tr("Select the server on which you want to register an account"));

	QLabel *lblServer = new QLabel(this);
	lblServer->setText(tr("Server:"));

	cmbServer = new QComboBox(this);
	cmbServer->setEditable(true);
	connect(cmbServer->lineEdit(),SIGNAL(textChanged(QString)),SIGNAL(completeChanged()));
	registerField(WF_REGISTER_DOMAIN"*",this,"accountDomain");

	QCompleter *serverCompleter = new QCompleter(this);
	serverCompleter->setModel(cmbServer->model());
	cmbServer->lineEdit()->setCompleter(serverCompleter);

	QLabel *lblServerList = new QLabel(this);
	lblServerList->setOpenExternalLinks(true);
	lblServerList->setText(QString("<a href='https://xmpp.net/directory.php'>%1</a>").arg(tr("Some public servers")));

	QLabel *lblNotice = new QLabel(this);
	lblNotice->setWordWrap(true);
	lblNotice->setText(tr("* Not all servers support within the client registration, in some cases, you can only register on the servers web site."));

	QHBoxLayout *hltServer = new QHBoxLayout;
	hltServer->addWidget(lblServer,0);
	hltServer->addWidget(cmbServer,1);
	hltServer->addWidget(lblServerList,0);

	cowConnOptions = new ConnectionOptionsWidget(this);
	registerField(WF_REGISTER_CONN_ENGINE,cowConnOptions,"connectionEngine");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addLayout(hltServer);
	layout->addWidget(lblNotice);
	layout->addSpacing(20);
	layout->addWidget(cowConnOptions);
	layout->setSpacing(layout->spacing()*2);
}

void RegisterServerPage::initializePage()
{
	static const QStringList servers = QStringList() 
		<< "jabbim.com" << "jabber.ru" << "xmpp.ru" << "jabber.cz" << "jabberpl.org"
		<< "richim.org" << "linuxlovers.at" << "palita.net" << "creep.im" << "draugr.de"
		<< "jabbim.pl" << "jabbim.cz" << "jabbim.hu"  << "jabbim.sk" << "jabster.pl"
		<< "njs.netlab.cz" << "is-a-furry.org" << "jabber.hot-chilli.net" << "jabber.at"
		<< "xmppnet.de" << "jabber.no" << "jabber.rueckgr.at" << "jabber.yeahnah.co.nz"
		<< "jabberes.org" << "suchat.org"	<< "chatme.im" << "tigase.im" << "ubuntu-jabber.de"
		<< "ubuntu-jabber.net" << "verdammung.org" << "xabber.de" << "xmpp-hosting.de" << "xmpp.jp";

	QWizardPage::initializePage();

	cmbServer->clear();
	cmbServer->addItem(tr("jabbim.com","Most stable and reliable server for your country which supports in-band account registration"));
	foreach (const QString &server, servers)
		if (cmbServer->findText(server) < 0)
			cmbServer->addItem(server);
	cmbServer->lineEdit()->selectAll();
}

bool RegisterServerPage::validatePage()
{
	cowConnOptions->applyOptions();
	return QWizardPage::validatePage();
}

QString RegisterServerPage::accountDomain() const
{
	return cmbServer->lineEdit()->text();
}

void RegisterServerPage::setAccountDomain(const QString &ADomain)
{
	int index = cmbServer->findText(ADomain);
	if (index < 0)
		cmbServer->lineEdit()->setText(ADomain);
	else
		cmbServer->setCurrentIndex(index);
	emit completeChanged();
}

void RegisterServerPage::saveAccountSettings(IAccount *AAccount) const
{
	cowConnOptions->saveOptions(AAccount);
	AAccount->setPassword(field(WF_REGISTER_PASSWORD).toString());
}


RegisterRequestPage::RegisterRequestPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Register on Server"));
	setSubTitle(tr("Fill out the form offered by server to register"));

	FReinitialize = false;
	dfwRegisterForm = NULL;

	FRegisterId = QString::null;
	registerField(WF_REGISTER_ID,this,"registerId");

	FXmppStream = NULL;
	registerField(WF_REGISTER_NODE,this,"accountNode");
	registerField(WF_REGISTER_PASSWORD,this,"accountPassword");

	lblCaption = new QLabel(this);
	lblCaption->setAlignment(Qt::AlignCenter);

	prbProgress = new QProgressBar(this);
	prbProgress->setRange(0,0);
	prbProgress->setTextVisible(false);
	prbProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	lblError = new QLabel(this);
	lblError->setWordWrap(true);
	lblError->setAlignment(Qt::AlignCenter);

	lblDescription = new QLabel(this);
	lblDescription->setWordWrap(true);
	lblDescription->setAlignment(Qt::AlignCenter);

	vltRegisterForm = new QVBoxLayout;
	vltRegisterForm->setMargin(0);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addStretch();
	layout->addWidget(lblCaption);
	layout->addWidget(prbProgress);
	layout->addWidget(lblError);
	layout->addWidget(lblDescription);
	layout->addLayout(vltRegisterForm);
	layout->addStretch();
	layout->setSpacing(layout->spacing()*2);

	FDataForms = PluginHelper::pluginInstance<IDataForms>();
	FRegistration = PluginHelper::pluginInstance<IRegistration>();

	if (FRegistration != NULL)
	{
		connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
			SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
		connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const XmppError &)),
			SLOT(onRegisterError(const QString &, const XmppError &)));
	}

	connect(AParent,SIGNAL(currentIdChanged(int)),SLOT(onWizardCurrentPageChanged(int)));
}

RegisterRequestPage::~RegisterRequestPage()
{
	if (FXmppStream)
		delete FXmppStream->instance();
}

void RegisterRequestPage::initializePage()
{
	QWizardPage::initializePage();
	FReinitialize = false;

	if (FXmppStream == NULL)
		FXmppStream = createXmppStream();
	else
		FXmppStream->abort(XmppError::null);

	if (dfwRegisterForm != NULL)
	{
		dfwRegisterForm->instance()->deleteLater();
		dfwRegisterForm = NULL;
	}

	if (FXmppStream!=NULL && FRegistration!=NULL && FDataForms!=NULL)
	{
		IConnection *conn = FXmppStream->connection();
		conn->engine()->loadConnectionSettings(conn,ACCOUNT_CONNECTION_OPTIONS);

		FXmppStream->setStreamJid(field(WF_REGISTER_DOMAIN).toString());

		FRegisterId = FRegistration->startStreamRegistration(FXmppStream);
	}

	if (!FRegisterId.isEmpty())
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Connecting...")));

		lblCaption->setVisible(true);
		prbProgress->setVisible(true);
		lblError->setVisible(false);
		lblDescription->setVisible(false);
	}
	else
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Failed to start registration :(")));
		lblError->setText(tr("Internal Error"));

		lblCaption->setVisible(true);
		prbProgress->setVisible(false);
		lblError->setVisible(true);
		lblDescription->setVisible(false);
	}
}

void RegisterRequestPage::cleanupPage()
{
	FRegisterId.clear();
	FRegisterFields = IRegisterFields();
	FRegisterSubmit = IRegisterSubmit();

	QWizardPage::cleanupPage();
}

bool RegisterRequestPage::isComplete() const
{
	return dfwRegisterForm!=NULL ? FDataForms->isSubmitValid(dfwRegisterForm->dataForm(),dfwRegisterForm->userDataForm()) : false;
}

bool RegisterRequestPage::validatePage()
{
	if (dfwRegisterForm != NULL)
	{
		FRegisterSubmit.key = FRegisterFields.key;
		FRegisterSubmit.serviceJid = FRegisterFields.serviceJid;
		if (FRegisterFields.fieldMask & IRegisterFields::Form)
		{
			FRegisterSubmit.form = FDataForms->dataSubmit(dfwRegisterForm->userDataForm());
			FRegisterSubmit.fieldMask = IRegisterFields::Form;
		}
		else
		{
			IDataForm form = dfwRegisterForm->userDataForm();
			FRegisterSubmit.username = FDataForms->fieldValue("username",form.fields).toString();
			FRegisterSubmit.password = FDataForms->fieldValue("password",form.fields).toString();
			FRegisterSubmit.email = FDataForms->fieldValue("email",form.fields).toString();
			FRegisterSubmit.fieldMask = FRegisterFields.fieldMask;
		}
		return FRegistration->submitStreamRegistration(FXmppStream,FRegisterSubmit) == FRegisterId;
	}
	return false;
}

QString RegisterRequestPage::registerId() const
{
	return FRegisterId;
}

void RegisterRequestPage::setRegisterId(const QString &AId)
{
	Q_UNUSED(AId);
}

QString RegisterRequestPage::accountNode() const
{
	return FXmppStream!=NULL ? FXmppStream->streamJid().node() : QString::null;
}

void RegisterRequestPage::setAccountNode(const QString &ANode)
{
	Q_UNUSED(ANode);
}

QString RegisterRequestPage::accountPassword() const
{
	return FXmppStream!=NULL ? FXmppStream->password() : QString::null;
}

void RegisterRequestPage::setAccountPassword(const QString &APassword)
{
	Q_UNUSED(APassword);
}

IXmppStream *RegisterRequestPage::createXmppStream() const
{
	IXmppStreamManager *xmppStreamManager = PluginHelper::pluginInstance<IXmppStreamManager>();
	IConnectionManager *connManager = PluginHelper::pluginInstance<IConnectionManager>();
	IConnectionEngine *connEngine = connManager!=NULL ? connManager->findConnectionEngine(field(WF_REGISTER_CONN_ENGINE).toString()) : NULL;
	if (xmppStreamManager!=NULL && connManager!=NULL && connEngine!=NULL)
	{
		IXmppStream *xmppStream = xmppStreamManager->createXmppStream(field(WF_REGISTER_DOMAIN).toString());
		xmppStream->setEncryptionRequired(true);

		IConnection *conn = connEngine->newConnection(ACCOUNT_CONNECTION_OPTIONS,xmppStream->instance());
		xmppStream->setConnection(conn);

		return xmppStream;
	}
	return NULL;
}

void RegisterRequestPage::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (FRegisterId == AId)
	{
		FRegisterFields = AFields;
		if ((AFields.fieldMask & IRegisterFields::Form) == 0)
		{
			FRegisterFields.form.type = DATAFORM_TYPE_FORM;
			FRegisterFields.form.instructions.append(AFields.instructions);
			if (AFields.fieldMask & IRegisterFields::Username)
			{
				IDataField field;
				field.var = "username";
				field.type = DATAFIELD_TYPE_TEXTSINGLE;
				field.label = tr("Username");
				field.required = true;
				field.value = FRegisterSubmit.username;
				FRegisterFields.form.fields.append(field);
			}
			if (AFields.fieldMask & IRegisterFields::Password)
			{
				IDataField field;
				field.var = "password";
				field.type = DATAFIELD_TYPE_TEXTPRIVATE;
				field.label = tr("Password");
				field.required = true;
				field.value = FRegisterSubmit.password;
				FRegisterFields.form.fields.append(field);
			}
			if (AFields.fieldMask & IRegisterFields::Email)
			{
				IDataField field;
				field.var = "email";
				field.type = DATAFIELD_TYPE_TEXTSINGLE;
				field.label = tr("Email");
				field.required = true;
				field.value = FRegisterSubmit.email;
				FRegisterFields.form.fields.append(field);
			}
		}
		else for (int i=0; i<FRegisterFields.form.fields.count(); i++)
		{
			QVariant value = FDataForms->fieldValue(FRegisterFields.form.fields.at(i).var,FRegisterSubmit.form.fields);
			if (!value.isNull())
				FRegisterFields.form.fields[i].value = value;
		}

		dfwRegisterForm = FDataForms->formWidget(FRegisterFields.form,this);
		vltRegisterForm->addWidget(dfwRegisterForm->instance());
		connect(dfwRegisterForm->instance(),SIGNAL(fieldChanged(IDataFieldWidget *)),SIGNAL(completeChanged()));

		lblCaption->setVisible(false);
		prbProgress->setVisible(false);
		lblError->setVisible(false);
		lblDescription->setVisible(false);

		emit completeChanged();
	}
}

void RegisterRequestPage::onRegisterError(const QString &AId, const XmppError &AError)
{
	if (FRegisterId == AId)
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Failed to register :(")));
		lblError->setText(AError.errorMessage());

		if (dfwRegisterForm != NULL)
		{
			dfwRegisterForm->instance()->deleteLater();
			dfwRegisterForm = NULL;
		}

		lblCaption->setVisible(true);
		lblError->setVisible(true);
		prbProgress->setVisible(false);
		lblDescription->setVisible(false);

		emit completeChanged();
	}
}

void RegisterRequestPage::onWizardCurrentPageChanged(int APage)
{
	if (APage == CreateAccountWizard::PageRegisterSubmit)
		FReinitialize = true;
	else if (APage==CreateAccountWizard::PageRegisterRequest && FReinitialize)
		initializePage();
}


RegisterSubmitPage::RegisterSubmitPage(QWidget *AParent) : QWizardPage(AParent)
{
	setFinalPage(true);
	setTitle(tr("Finishing Registration"));
	setSubTitle(tr("Wizard waiting for registration confirmation from server"));

	lblCaption = new QLabel(this);
	lblCaption->setAlignment(Qt::AlignCenter);

	prbProgress = new QProgressBar(this);
	prbProgress->setRange(0,0);
	prbProgress->setTextVisible(false);
	prbProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	lblError = new QLabel(this);
	lblError->setWordWrap(true);
	lblError->setAlignment(Qt::AlignCenter);

	lblDescription = new QLabel(this);
	lblDescription->setWordWrap(true);
	lblDescription->setAlignment(Qt::AlignCenter);

	chbShowSettings = new QCheckBox(this);
	chbShowSettings->setText(tr("Show account settings window"));
	registerField(WF_REGISTER_SHOW_SETTINGS,chbShowSettings);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addStretch();
	layout->addWidget(lblCaption);
	layout->addWidget(prbProgress);
	layout->addWidget(lblError);
	layout->addWidget(lblDescription);
	layout->addStretch();
	layout->addWidget(chbShowSettings);
	layout->setSpacing(layout->spacing()*2);

	FRegistration = PluginHelper::pluginInstance<IRegistration>();
	if (FRegistration != NULL)
	{
		connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const XmppError &)),
			SLOT(onRegisterError(const QString &, const XmppError &)));
		connect(FRegistration->instance(),SIGNAL(registerSuccess(const QString &)),
			SLOT(onRegisterSuccess(const QString &)));
	}
}

void RegisterSubmitPage::initializePage()
{
	QWizardPage::initializePage();

	lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Registration...")));

	lblCaption->setVisible(true);
	prbProgress->setVisible(true);
	lblError->setVisible(false);
	lblDescription->setVisible(false);
	chbShowSettings->setVisible(false);

	FRegistered = false;
}

bool RegisterSubmitPage::isComplete() const
{
	return FRegistered && QWizardPage::isComplete();
}

int RegisterSubmitPage::nextId() const
{
	return -1;
}

void RegisterSubmitPage::onRegisterError(const QString &AId, const XmppError &AError)
{
	if (field(WF_REGISTER_ID).toString() == AId)
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Failed to register :(")));

		if (AError.toStanzaError().conditionCode() == XmppStanzaError::EC_CONFLICT)
			lblError->setText(tr("This username is already registered by someone else"));
		else
			lblError->setText(AError.errorMessage());

		lblCaption->setVisible(true);
		lblError->setVisible(true);
		prbProgress->setVisible(false);
		lblDescription->setVisible(false);
		chbShowSettings->setVisible(false);

		emit completeChanged();
	}
}

void RegisterSubmitPage::onRegisterSuccess(const QString &AId)
{
	if (field(WF_REGISTER_ID).toString() == AId)
	{
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("You have successfully registered!")));
		lblDescription->setText(tr("Account %1@%2 successfully registered, click 'Finish' button to add the account.").arg(field(WF_REGISTER_NODE).toString(),field(WF_REGISTER_DOMAIN).toString()));

		lblCaption->setVisible(true);
		lblError->setVisible(false);
		prbProgress->setVisible(false);
		lblDescription->setVisible(true);
		chbShowSettings->setVisible(true);

		FRegistered = true;
		emit completeChanged();
	}
}
