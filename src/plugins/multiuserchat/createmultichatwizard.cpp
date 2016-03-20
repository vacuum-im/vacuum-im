#include "createmultichatwizard.h"

#include <QSpacerItem>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QInputDialog>
#include <QMessageBox>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/statisticsparams.h>
#include <utils/pluginhelper.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/logger.h>

#define WF_MODE                   "Mode"
#define WF_CONFIG_HINTS           "ConfigHints"

#define WF_ACCOUNT                "Account"
#define WF_SERVER                 "Server"
#define WF_SERVICE                "Service"
#define WF_ROOM_JID               "RoomJid"
#define WF_ROOM_NICK              "RoomNick"
#define WF_ROOM_PASSWORD          "RoomPassword"

#define WF_MANUAL_ACCOUNT         "ManualAccount"
#define WF_MANUAL_ROOM_JID        "ManualRoomJid"
#define WF_MANUAL_ROOM_NICK       "ManualRoomNick"
#define WF_MANUAL_ROOM_PASSWORD   "ManualRoomPassword"

#define OFV_LAST_ACCOUNT          "muc.create-multichat-wizard.last-account"
#define OFV_LAST_SERVER           "muc.create-multichat-wizard.last-server"
#define OFV_LAST_SERVICE          "muc.create-multichat-wizard.last-service"
#define OFV_USER_SERVERS          "muc.create-multichat-wizard.user-servers"
#define OFV_LAST_NICK             "muc.create-multichat-wizard.last-nick"

#define DIC_CONFERENCE            "conference"
#define DIT_CONFERENCE_TEXT       "text"


/***********************
 * CreateMultiChatWizard
 ***********************/
CreateMultiChatWizard::CreateMultiChatWizard(QWidget *AParent) : QWizard(AParent)
{
	initialize();
	setStartId(PageMode);
}

CreateMultiChatWizard::CreateMultiChatWizard(Mode AMode, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent) : QWizard(AParent)
{
	initialize();

	setField(WF_MODE,AMode);
	if (AMode == CreateMultiChatWizard::ModeManual)
	{
		if (AStreamJid.isValid())
			setField(WF_MANUAL_ACCOUNT,AStreamJid.full());
		if (ARoomJid.hasNode())
			setField(WF_ROOM_JID,ARoomJid.bare());
		if (!ANick.isEmpty())
			setField(WF_ROOM_NICK,ANick);
		if (!APassword.isEmpty())
			setField(WF_ROOM_PASSWORD,APassword);
		setStartId(PageManual);
	}
	else
	{
		if (AStreamJid.isValid())
			setField(WF_ACCOUNT,AStreamJid.full());
		if (AStreamJid.isValid())
			setField(WF_SERVER,AStreamJid.domain());
		if (ARoomJid.hasDomain())
			setField(WF_SERVICE,ARoomJid.domain());
		if (ARoomJid.hasNode())
			setField(WF_ROOM_JID,ARoomJid.bare());
		if (!ANick.isEmpty())
			setField(WF_ROOM_NICK,ANick);
		if (!APassword.isEmpty())
			setField(WF_ROOM_PASSWORD,APassword);

		if (!AStreamJid.isValid())
			setStartId(PageService);
		else if (!ARoomJid.isValid())
			setStartId(PageService);
		else if (!ARoomJid.hasNode())
			setStartId(PageRoom);
		else if (AMode == ModeJoin)
			setStartId(PageJoin);
		else if (AMode == ModeCreate)
			setStartId(PageConfig);
	}
}

void CreateMultiChatWizard::setConfigHints(const QMap<QString,QVariant> &AHints)
{
	setField(WF_CONFIG_HINTS,AHints);
}

void CreateMultiChatWizard::accept()
{
	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	if (multiChatManager)
	{
		int wizardMode = field(WF_MODE).toInt();
		bool isManualMode = wizardMode==CreateMultiChatWizard::ModeManual;
		QString streamJid = !isManualMode ? field(WF_ACCOUNT).toString() : field(WF_MANUAL_ACCOUNT).toString();
		QString serverJid = !isManualMode ? field(WF_SERVER).toString() : QString::null;
		QString serviceJid = !isManualMode ? field(WF_SERVICE).toString() : QString::null;
		QString roomJid = !isManualMode ? field(WF_ROOM_JID).toString() : field(WF_MANUAL_ROOM_JID).toString();
		QString roomNick = !isManualMode ? field(WF_ROOM_NICK).toString(): field(WF_MANUAL_ROOM_NICK).toString();
		QString roomPassword = !isManualMode ? field(WF_ROOM_PASSWORD).toString(): field(WF_MANUAL_ROOM_PASSWORD).toString();
		
		if (!streamJid.isEmpty() && !roomJid.isEmpty() && !roomNick.isEmpty())
		{
			LOG_STRM_INFO(streamJid,QString("Entering conference by wizard, room=%1, nick=%2").arg(roomJid, roomNick));

			IMultiUserChatWindow *window = multiChatManager->getMultiChatWindow(streamJid,roomJid,roomNick,roomPassword);
			if (window)
			{
				if (wizardMode == CreateMultiChatWizard::ModeJoin)
					REPORT_EVENT(SEVP_MUC_WIZARD_JOIN,1)
				else if (wizardMode == CreateMultiChatWizard::ModeCreate)
					REPORT_EVENT(SEVP_MUC_WIZARD_CREATE,1)
				else if (wizardMode == CreateMultiChatWizard::ModeManual)
					REPORT_EVENT(SEVP_MUC_WIZARD_MANUAL,1)

				if (window->multiUserChat()->nickname() != roomNick)
					window->multiUserChat()->setNickname(roomNick);
				else
					window->multiUserChat()->sendStreamPresence();
				window->showTabPage();

				if (!serverJid.isEmpty())
					Options::setFileValue(serverJid,OFV_LAST_SERVER);
				if (!serviceJid.isEmpty())
					Options::setFileValue(serviceJid,OFV_LAST_SERVICE);
				Options::setFileValue(streamJid,OFV_LAST_ACCOUNT);
				Options::setFileValue(roomNick,OFV_LAST_NICK);

				emit wizardAccepted(window);
				QDialog::accept();
			}
			else
			{
				REPORT_ERROR("Failed to join to the conference: Conference windows is not created");
				QMessageBox::critical(this,tr("Error"),tr("Failed to join to the conference: Conference windows is not created"));
			}
		}
		else
		{
			REPORT_ERROR("Failed to join to the conference: Not all required parameters is specified");
			QMessageBox::critical(this,tr("Error"),tr("Failed to join to the conference: Not all required parameters is specified"));
		}
	}
	else
	{
		REPORT_ERROR("Failed to join to the conference: Required interface is not found");
	}
}

void CreateMultiChatWizard::initialize()
{
	REPORT_VIEW;
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Create Conference Wizard"));
#ifndef Q_WS_MAC
	setWizardStyle(QWizard::ModernStyle);
#endif
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_JOIN,0,0,"windowIcon");

	setPage(PageMode, new ModePage(this));
	setPage(PageService, new ServicePage(this));
	setPage(PageRoom, new RoomPage(this));
	setPage(PageConfig, new ConfigPage(this));
	setPage(PageJoin, new JoinPage(this));
	setPage(PageManual, new ManualPage(this));
}

/**********
 * ModePage
 **********/
ModePage::ModePage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Join to the conference or create a new one"));
	setSubTitle(tr("This wizard will help you to join to existing conference or create a new one"));

	rbtJoinRoom = new QRadioButton(this);
	rbtJoinRoom->setText(tr("I want to join to the existing conference"));

	rbtCreateRoom = new QRadioButton(this);
	rbtCreateRoom->setText(tr("I want to create a new conference"));

	rbtManuallyRoom = new QRadioButton(this);
	rbtManuallyRoom->setText(tr("I want manually specify all parameters to join or create the conference"));

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(rbtJoinRoom);
	layout->addWidget(rbtCreateRoom);
	layout->addWidget(rbtManuallyRoom);
	layout->setMargin(0);

	QWidget::setTabOrder(rbtJoinRoom, rbtCreateRoom);

	registerField(WF_MODE,this,"wizardMode");
}

void ModePage::initializePage()
{
	setWizardMode(CreateMultiChatWizard::ModeJoin);
}

int ModePage::nextId() const
{
	switch (wizardMode())
	{
	case CreateMultiChatWizard::ModeJoin:
	case CreateMultiChatWizard::ModeCreate:
		return CreateMultiChatWizard::PageService;
	case CreateMultiChatWizard::ModeManual:
		return CreateMultiChatWizard::PageManual;
	default:
		return -1;
	}
}

int ModePage::wizardMode() const
{
	if (rbtJoinRoom->isChecked())
		return CreateMultiChatWizard::ModeJoin;
	else if (rbtCreateRoom->isChecked())
		return CreateMultiChatWizard::ModeCreate;
	else if (rbtManuallyRoom->isChecked())
		return CreateMultiChatWizard::ModeManual;
	return -1;
}

void ModePage::setWizardMode(int AMode)
{
	if (AMode == CreateMultiChatWizard::ModeJoin)
		rbtJoinRoom->setChecked(true);
	else if (AMode == CreateMultiChatWizard::ModeCreate)
		rbtCreateRoom->setChecked(true);
	else if (AMode == CreateMultiChatWizard::ModeManual)
		rbtManuallyRoom->setChecked(true);
}

/*************
 * ServicePage
 *************/
ServicePage::ServicePage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Conference location"));

	FWaitItems = false;

	lblAccount = new QLabel(this);
	lblAccount->setWordWrap(true);

	lblServer = new QLabel(this);
	lblServer->setWordWrap(true);

	lblService = new QLabel(this);
	lblService->setWordWrap(true);

	lblInfo = new QLabel(this);
	lblInfo->setWordWrap(true);
	lblInfo->setTextFormat(Qt::PlainText);

	cmbAccount = new QComboBox(this);
	cmbServer = new QComboBox(this);
	cmbService = new QComboBox(this);

	QToolButton *tlbAddServer = new QToolButton(this);
	tlbAddServer->setText(tr("Add..."));
	connect(tlbAddServer,SIGNAL(clicked()),SLOT(onAddServerButtonClicked()));

	QHBoxLayout *lytAccount = new QHBoxLayout();
	lytAccount->addWidget(new QLabel(tr("Account:")));
	lytAccount->addWidget(cmbAccount,1);

	QHBoxLayout *lytServer = new QHBoxLayout();
	lytServer->addWidget(new QLabel(tr("Server:")));
	lytServer->addWidget(cmbServer,1);
	lytServer->addWidget(tlbAddServer);

	QHBoxLayout *lytService = new QHBoxLayout();
	lytService->addWidget(new QLabel(tr("Service:")));
	lytService->addWidget(cmbService,1);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(lblAccount);
	layout->addLayout(lytAccount);
	layout->addSpacing(10);
	layout->addWidget(lblServer);
	layout->addLayout(lytServer);
	layout->addSpacing(10);
	layout->addWidget(lblService);
	layout->addLayout(lytService);
	layout->addSpacing(10);
	layout->addWidget(lblInfo);
	layout->setMargin(0);

	QWidget::setTabOrder(cmbAccount,cmbServer);
	QWidget::setTabOrder(cmbServer,tlbAddServer);
	QWidget::setTabOrder(tlbAddServer,cmbService);

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	if (discovery)
	{
		connect(discovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoRecieved(const IDiscoInfo &)));
		connect(discovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsRecieved(const IDiscoItems &)));
	}

	IAccountManager *accountManager = PluginHelper::pluginInstance<IAccountManager>();
	if (accountManager)
	{
		QMap<int, IAccount *> orderedAccounts;
		foreach(IAccount *account, accountManager->accounts())
		{
			if (account->isActive())
				orderedAccounts.insert(account->accountOrder(), account);
		}

		foreach(IAccount *account, orderedAccounts)
		{
			if (account->xmppStream()->isOpen())
				cmbAccount->addItem(account->name(), account->streamJid().pFull());
			if (cmbServer->findData(account->streamJid().pDomain()) < 0)
				cmbServer->addItem(account->streamJid().domain(), account->streamJid().pDomain());
		}
	}
	
	foreach(const Jid &server, Options::fileValue(OFV_USER_SERVERS).toStringList())
	{
		if (cmbServer->findData(server.pDomain()) < 0)
			cmbServer->addItem(server.domain(), server.pDomain());
	}

	int lastAccountIndex = cmbAccount->findData(Options::fileValue(OFV_LAST_ACCOUNT));
	if (lastAccountIndex >= 0)
		cmbAccount->setCurrentIndex(lastAccountIndex);

	int lastServerIndex = cmbServer->findData(Options::fileValue(OFV_LAST_SERVER));
	if (lastServerIndex >= 0)
		cmbServer->setCurrentIndex(lastServerIndex);

	connect(cmbAccount,SIGNAL(currentIndexChanged(int)),SLOT(onCurrentAccountChanged()));
	connect(cmbServer,SIGNAL(currentIndexChanged(int)),SLOT(onCurrentServerChanged()));
	connect(cmbService,SIGNAL(currentIndexChanged(int)),SLOT(onCurrentServiceChanged()));

	registerField(WF_ACCOUNT,this,"streamJid");
	registerField(WF_SERVER,this,"serverJid");
	registerField(WF_SERVICE,this,"serviceJid");
}

void ServicePage::initializePage()
{
	if (wizardMode() == CreateMultiChatWizard::ModeJoin)
	{
		setSubTitle(tr("Select account, server and service to join to the conference"));
		lblAccount->setText(tr("Select the account to join to the conference"));
		lblServer->setText(tr("You can join to the conference located at almost any Jabber-server, select one from the list or add your own"));
		lblService->setText(tr("Each Jabber-server can have multiple conference services, select one of the available"));
	}
	else if (wizardMode() == CreateMultiChatWizard::ModeCreate)
	{
		setSubTitle(tr("Select account, server, and service to create the conference"));
		lblAccount->setText(tr("Select the account to create a conference"));
		lblServer->setText(tr("You can create a conference at almost any Jabber-server, select one from the list or add your own"));
		lblService->setText(tr("Each Jabber-server can have multiple conference services, select one of the available"));
	}

	onCurrentAccountChanged();
}

bool ServicePage::isComplete() const
{
	if (cmbAccount->currentIndex() < 0)
		return false;
	if (cmbServer->currentIndex() < 0)
		return false;
	if (cmbService->currentIndex() < 0)
		return false;
	return QWizardPage::isComplete();
}

int ServicePage::nextId() const
{
	return CreateMultiChatWizard::PageRoom;
}

int ServicePage::wizardMode() const
{
	return field(WF_MODE).toInt();
}

QString ServicePage::streamJid() const
{
	return cmbAccount->itemData(cmbAccount->currentIndex()).toString();
}

void ServicePage::setStreamJid(const QString &AStreamJid)
{
	Jid stream = AStreamJid;
	cmbAccount->setCurrentIndex(cmbAccount->findData(stream.pFull()));
}

QString ServicePage::serverJid() const
{
	return cmbServer->itemData(cmbServer->currentIndex()).toString();
}

void ServicePage::setServerJid(const QString &AServerJid)
{
	Jid server = AServerJid;
	int index = cmbServer->findData(server.pDomain());
	if (index < 0)
	{
		cmbServer->addItem(server.domain(),server.pDomain());
		cmbServer->setCurrentIndex(cmbServer->count() - 1);
	}
	else
	{
		cmbServer->setCurrentIndex(index);
	}
}

QString ServicePage::serviceJid() const
{
	return cmbService->itemData(cmbService->currentIndex()).toString();
}

void ServicePage::setServiceJid(const QString &AServiceJid)
{
	Jid service = AServiceJid;
	int index = cmbService->findData(service.pDomain());
	if (index < 0)
	{
		cmbService->addItem(service.domain(),service.pDomain());
		cmbService->setCurrentIndex(cmbService->count() - 1);
	}
	else
	{
		cmbService->setCurrentIndex(index);
	}
}

void ServicePage::processDiscoInfo(const IDiscoInfo &AInfo)
{
	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	int index = discovery!=NULL ? discovery->findIdentity(AInfo.identity,DIC_CONFERENCE,DIT_CONFERENCE_TEXT) : -1;
	if (index>=0 && AInfo.error.isNull() && cmbService->findData(AInfo.contactJid.pDomain())<0)
	{
		IDiscoIdentity ident = AInfo.identity.value(index);
		if (!ident.name.isEmpty())
			cmbService->addItem(QString("%1 (%2)").arg(ident.name.trimmed(),AInfo.contactJid.domain()), AInfo.contactJid.pDomain());
		else
			cmbService->addItem(AInfo.contactJid.domain(), AInfo.contactJid.pDomain());
		emit completeChanged();
	}
	
	if (!FWaitInfo.isEmpty())
		lblInfo->setText(tr("Searching for conference services (%1)...").arg(FWaitInfo.count()));
	else if (cmbService->count() == 0)
		lblInfo->setText(tr("Conference services are not found on this server"));
	else
		lblInfo->setText(QString::null);
}

void ServicePage::onCurrentAccountChanged()
{
	onCurrentServerChanged();
}

void ServicePage::onCurrentServerChanged()
{
	FWaitInfo.clear();
	cmbService->clear();
	lblInfo->setText(QString::null);

	if (cmbAccount->count()>0 && cmbServer->count()>0)
	{
		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery && discovery->requestDiscoItems(streamJid(),serverJid()))
		{
			FWaitItems = true;
			lblInfo->setText(tr("Loading list of available services..."));
		}
		else
		{
			lblInfo->setText(tr("Failed to load list of services"));
		}
	}
	else
	{
		lblInfo->setText(tr("Account or server is not selected"));
	}

	emit completeChanged();
}

void ServicePage::onCurrentServiceChanged()
{
	emit completeChanged();
}

void ServicePage::onAddServerButtonClicked()
{
	Jid server = QInputDialog::getText(this,tr("Append Server"),tr("Enter server domain:"));
	if (server.isValid())
	{
		if (cmbServer->findData(server.pDomain()) < 0)
		{
			QStringList userServers = Options::fileValue(OFV_USER_SERVERS).toStringList();
			if (!userServers.contains(server.pDomain()))
			{
				userServers.prepend(server.pDomain());
				Options::setFileValue(userServers,OFV_USER_SERVERS);
			}
		}
		setServerJid(server.domain());
	}
}

void ServicePage::onDiscoInfoRecieved(const IDiscoInfo &AInfo)
{
	if (FWaitInfo.contains(AInfo.contactJid) && AInfo.streamJid==streamJid() && AInfo.node.isEmpty())
	{
		FWaitInfo.removeAll(AInfo.contactJid);
		processDiscoInfo(AInfo);
	}
}

void ServicePage::onDiscoItemsRecieved(const IDiscoItems &AItems)
{
	if (FWaitItems && AItems.streamJid==streamJid() && AItems.contactJid==serverJid() && AItems.node.isEmpty())
	{
		FWaitItems = false;
		if (AItems.error.isNull())
		{
			IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
			foreach(const IDiscoItem &item, AItems.items)
			{
				if (discovery->hasDiscoInfo(AItems.streamJid,item.itemJid))
					processDiscoInfo(discovery->discoInfo(AItems.streamJid,item.itemJid));
				else if (discovery->requestDiscoInfo(AItems.streamJid,item.itemJid))
					FWaitInfo.append(item.itemJid);
			}
			processDiscoInfo(IDiscoInfo());
		}
		else
		{
			lblInfo->setText(tr("Failed to load a list of services: %1").arg(AItems.error.errorMessage()));
		}
	}
}

/*************
 * RoomPage
 *************/
RoomPage::RoomPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Conference name"));

	FWaitInfo = false;
	FWaitItems = false;
	FRoomChecked = false;

	sleSearch = new SearchLineEdit(this);
	sleSearch->setPlaceholderText(tr("Search conferences"));
	connect(sleSearch,SIGNAL(searchStart()),SLOT(onRoomSearchStart()));

	FRoomModel = new QStandardItemModel(this);
	FRoomModel->setColumnCount(RMC__COUNT);
	FRoomModel->setHorizontalHeaderLabels(QStringList() << tr("Title") << QString::null);

	FRoomProxy = new QSortFilterProxyModel(FRoomModel);
	FRoomProxy->setSourceModel(FRoomModel);
	FRoomProxy->setSortLocaleAware(true);
	FRoomProxy->setSortRole(RMR_SORT);
	FRoomProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

	tbvRoomView = new QTableView(this);
	tbvRoomView->setModel(FRoomProxy);
	tbvRoomView->setSortingEnabled(true);
	tbvRoomView->setAlternatingRowColors(true);
	tbvRoomView->setEditTriggers(QTreeView::NoEditTriggers);
	tbvRoomView->setSelectionBehavior(QTreeView::SelectRows);
	tbvRoomView->setSelectionMode(QTableView::SingleSelection);
	tbvRoomView->verticalHeader()->hide();
	tbvRoomView->horizontalHeader()->setHighlightSections(false);
	tbvRoomView->horizontalHeader()->setResizeMode(RMC_NAME,QHeaderView::Stretch);
	tbvRoomView->horizontalHeader()->setResizeMode(RMC_USERS,QHeaderView::ResizeToContents);
	tbvRoomView->horizontalHeader()->setSortIndicator(RMC_NAME,Qt::AscendingOrder);
	connect(tbvRoomView->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),SLOT(onCurrentRoomChanged(const QModelIndex &, const QModelIndex &)));

	lblRoomNode = new QLabel(this);

	lneRoomNode = new QLineEdit(this);
	connect(lneRoomNode,SIGNAL(textChanged(const QString &)),SLOT(onRoomNodeTextChanged()));

	lblRoomDomain = new QLabel(this);
	lblRoomDomain->setTextFormat(Qt::PlainText);

	lblInfo = new QLabel(this);
	lblInfo->setWordWrap(true);
	lblInfo->setTextFormat(Qt::PlainText);

	FRoomNodeTimer.setSingleShot(true);
	connect(&FRoomNodeTimer,SIGNAL(timeout()),SLOT(onRoomNodeTimerTimeout()));

	QHBoxLayout *nodeLayout = new QHBoxLayout;
	nodeLayout->addWidget(lblRoomNode);
	nodeLayout->addWidget(lneRoomNode,1);
	nodeLayout->addWidget(lblRoomDomain);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(sleSearch);
	layout->addWidget(tbvRoomView);
	layout->addLayout(nodeLayout);
	layout->addWidget(lblInfo);
	layout->setMargin(0);

	QWidget::setTabOrder(lneRoomNode, sleSearch);
	QWidget::setTabOrder(sleSearch, tbvRoomView);

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	if (discovery)
	{
		connect(discovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoRecieved(const IDiscoInfo &)));
		connect(discovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsRecieved(const IDiscoItems &)));
	}

	registerField(WF_ROOM_JID,this,"roomJid");
}

void RoomPage::initializePage()
{
	lblRoomDomain->setText("@" + serviceJid().domain());

	if (wizardMode() == CreateMultiChatWizard::ModeJoin)
	{
		sleSearch->setVisible(true);
		tbvRoomView->setVisible(true);
		lblRoomNode->setText(tr("Join to the conference:"));
		setSubTitle(tr("Select a conference from the list or explicitly specify the name of the hidden one"));

		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery && discovery->requestDiscoItems(streamJid(),serviceJid()))
		{
			FWaitItems = true;
			FRoomModel->setRowCount(0);
			lblInfo->setText(tr("Loading list of conferences..."));
		}
		else
		{
			lblInfo->setText(tr("Failed to load list of conferences"));
		}
	}
	else if (wizardMode() == CreateMultiChatWizard::ModeCreate)
	{
		sleSearch->setVisible(false);
		tbvRoomView->setVisible(false);
		lblRoomNode->setText(tr("Create the conference:"));
		setSubTitle(tr("Enter unique name for the new conference"));
	}

	onRoomNodeTextChanged();
}

bool RoomPage::isComplete() const
{
	if (!FRoomChecked)
		return false;
	if (lneRoomNode->text().isEmpty())
		return false;
	return QWizardPage::isComplete();
}

int RoomPage::nextId() const
{
	switch (wizardMode())
	{
	case CreateMultiChatWizard::ModeJoin:
		return CreateMultiChatWizard::PageJoin;
	case CreateMultiChatWizard::ModeCreate:
		return CreateMultiChatWizard::PageConfig;
	default:
		return -1;
	}
}

int RoomPage::wizardMode() const
{
	return field(WF_MODE).toInt();
}

Jid RoomPage::streamJid() const
{
	return field(WF_ACCOUNT).toString();
}

Jid RoomPage::serviceJid() const
{
	return field(WF_SERVICE).toString();
}

QString RoomPage::roomJid() const
{
	return !lneRoomNode->text().isEmpty() ? Jid::fromUserInput(lneRoomNode->text() + "@" + field(WF_SERVICE).toString()).pBare() : QString::null;
}

void RoomPage::setRoomJid(const QString &ARoomJid)
{
	Jid room = ARoomJid;
	lneRoomNode->setText(room.uNode());
}

void RoomPage::onRoomSearchStart()
{
	FRoomProxy->setFilterFixedString(sleSearch->text());
	tbvRoomView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
}

void RoomPage::onRoomNodeTextChanged()
{
	FRoomNodeTimer.start(500);

	FRoomChecked = false;
	emit completeChanged();
}

void RoomPage::onRoomNodeTimerTimeout()
{
	Jid room = roomJid();
	if (room.isValid())
	{
		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery && discovery->requestDiscoInfo(streamJid(),room))
		{
			FWaitInfo = true;
			if (wizardMode() == CreateMultiChatWizard::ModeJoin)
				lblInfo->setText(tr("Loading conference description..."));
			else if (wizardMode() == CreateMultiChatWizard::ModeCreate)
				lblInfo->setText(tr("Checking conference existence..."));
		}
		else
		{
			if (wizardMode() == CreateMultiChatWizard::ModeJoin)
				lblInfo->setText(tr("Failed to load conference description"));
			else if (wizardMode() == CreateMultiChatWizard::ModeCreate)
				lblInfo->setText(tr("Failed to check conference existence"));
		}
	}
	else if (!room.isEmpty())
	{
		lblInfo->setText(tr("Invalid conference name"));
	}
	else
	{
		lblInfo->setText(QString::null);
	}
}

void RoomPage::onDiscoInfoRecieved(const IDiscoInfo &AInfo)
{
	if (FWaitInfo && AInfo.streamJid==streamJid() && AInfo.contactJid==roomJid() && AInfo.node.isEmpty())
	{
		FWaitInfo = false;
		if (wizardMode() == CreateMultiChatWizard::ModeJoin)
		{
			if (AInfo.error.isNull())
			{
				IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
				int index = discovery!=NULL ? discovery->findIdentity(AInfo.identity,DIC_CONFERENCE,DIT_CONFERENCE_TEXT) : -1;
				if (index >= 0)
				{
					IDiscoIdentity ident = AInfo.identity.value(index);
					lblInfo->setText(!ident.name.isEmpty() ? ident.name.trimmed() : AInfo.contactJid.uNode());

					FRoomChecked = true;
					emit completeChanged();
				}
				else
				{
					lblInfo->setText(tr("Conference description is not available or invalid"));
				}
			}
			else
			{
				lblInfo->setText(tr("Failed to load conference description: %1").arg(AInfo.error.errorMessage()));
			}
		}
		else if (wizardMode() == CreateMultiChatWizard::ModeCreate)
		{
			if (AInfo.error.isNull())
			{
				lblInfo->setText(tr("Conference '%1@%2' already exists, choose another name").arg(lneRoomNode->text(), serviceJid().domain()));
			}
			else if (AInfo.error.conditionCode() == XmppStanzaError::EC_ITEM_NOT_FOUND)
			{
				lblInfo->setText(QString::null);

				FRoomChecked = true;
				emit completeChanged();
			}
			else
			{
				lblInfo->setText(tr("Failed to check conference existence: %1").arg(AInfo.error.errorMessage()));
			}
		}
	}
}

void RoomPage::onDiscoItemsRecieved(const IDiscoItems &AItems)
{
	if (FWaitItems && AItems.streamJid==streamJid() && AItems.contactJid==serviceJid() && AItems.node.isEmpty())
	{
		FWaitItems = false;
		if (AItems.error.isNull())
		{
			foreach(const IDiscoItem &discoItem, AItems.items)
			{
				QStandardItem *nameItem = new QStandardItem;
				nameItem->setData(discoItem.itemJid.pBare(), RMR_ROOM_JID);

				QStandardItem *usersItem = new QStandardItem;
				usersItem->setData(0, RMR_SORT);

				if (!discoItem.name.isEmpty())
				{
					QRegExp rx("\\((\\d+)\\)$");

					int pos = rx.indexIn(discoItem.name);
					if (pos != -1)
					{
						nameItem->setText(QString("%1 (%2)").arg(discoItem.name.left(pos).trimmed(),discoItem.itemJid.uNode()));
						usersItem->setText(rx.cap(1));
						usersItem->setData(rx.cap(1).toInt(), RMR_SORT);
					}
					else
					{
						nameItem->setText(QString("%1 (%2)").arg(discoItem.name.trimmed(),discoItem.itemJid.uNode()));
					}
				}
				else
				{
					nameItem->setText(discoItem.itemJid.uBare());
				}
				nameItem->setData(nameItem->text(), RMR_SORT);
				nameItem->setData(nameItem->text(), Qt::ToolTip);

				FRoomModel->appendRow(QList<QStandardItem *>() << nameItem << usersItem);
			}
			lblInfo->setText(QString::null);
			tbvRoomView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
			FRoomProxy->sort(tbvRoomView->horizontalHeader()->sortIndicatorSection(), tbvRoomView->horizontalHeader()->sortIndicatorOrder());
		}
		else
		{
			lblInfo->setText(tr("Failed to load list of conferences: %1").arg(AItems.error.errorMessage()));
		}
	}
}

void RoomPage::onCurrentRoomChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious)
{
	Q_UNUSED(APrevious);
	QStandardItem *item = FRoomModel->itemFromIndex(FRoomProxy->mapToSource(ACurrent));
	item = item!=NULL ? FRoomModel->item(item->row(),RMC_NAME) : NULL;
	if (item)
	{
		Jid room = item->data(RMR_ROOM_JID).toString();
		lneRoomNode->setText(room.uNode());
		FRoomNodeTimer.start(0);
	}
}

/*************
 * CreatePage
 *************/
ConfigPage::ConfigPage(QWidget *AParent) : QWizardPage(AParent)
{
	setTitle(tr("Conference settings"));
	setSubTitle(tr("Enter the desired parameters of the new conference"));

	FMultiChat = NULL;
	FRoomCreated = false;
	FRoomConfigured = false;
	FConfigFormWidget = NULL;
	FRoomNick = QUuid::createUuid().toString();

	lblCaption = new QLabel(this);
	lblCaption->setTextFormat(Qt::RichText);
	lblCaption->setAlignment(Qt::AlignCenter);

	wdtConfig = new QWidget(this);
	wdtConfig->setLayout(new QVBoxLayout);
	wdtConfig->layout()->setMargin(0);

	prbProgress = new QProgressBar(this);
	prbProgress->setRange(0,0);
	prbProgress->setTextVisible(false);
	prbProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	lblInfo = new QLabel(this);
	lblInfo->setWordWrap(true);
	lblInfo->setTextFormat(Qt::PlainText);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addStretch();
	layout->addWidget(lblCaption);
	layout->addWidget(wdtConfig);
	layout->addWidget(prbProgress);
	layout->addWidget(lblInfo);
	layout->addStretch();
	layout->setMargin(0);

	registerField(WF_CONFIG_HINTS,this,"configHints");
}

void ConfigPage::cleanupPage()
{
	if (FMultiChat != NULL)
	{
		if (FRoomCreated)
			FMultiChat->destroyRoom(QString::null);
		delete FMultiChat->instance();
		FMultiChat = NULL;
	}
	QWizardPage::cleanupPage();
}

void ConfigPage::initializePage()
{
	FRoomCreated = false;
	FRoomConfigured = false;

	lblCaption->setVisible(true);
	prbProgress->setVisible(true);
	wdtConfig->setVisible(false);

	lblInfo->setText(QString::null);
	lblInfo->setAlignment(Qt::AlignCenter);

	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	FMultiChat = multiChatManager!=NULL ? multiChatManager->getMultiUserChat(streamJid(),roomJid(),FRoomNick,QString::null,false) : NULL;
	if (FMultiChat != NULL)
	{
		FMultiChat->instance()->setParent(this);
		connect(FMultiChat->instance(),SIGNAL(stateChanged(int)),SLOT(onMultiChatStateChanged(int)));
		connect(FMultiChat->instance(),SIGNAL(roomConfigLoaded(const QString &, const IDataForm &)),SLOT(onMultiChatConfigLoaded(const QString &, const IDataForm &)));
		connect(FMultiChat->instance(),SIGNAL(roomConfigUpdated(const QString &, const IDataForm &)),SLOT(onMultiChatConfigUpdated(const QString &, const IDataForm &)));
		connect(FMultiChat->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onMultiChatRequestFailed(const QString &, const XmppError &)));

		if (FMultiChat->sendStreamPresence())
			lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Creating conference...")));
		else
			setError(tr("Failed to create conference"));
	}
	else
	{
		setError(tr("Failed to create conference instance"));
	}
}

bool ConfigPage::isComplete() const
{
	if (!FRoomCreated)
		return false;
	if (!FConfigLoadRequestId.isEmpty())
		return false;
	if (!FConfigUpdateRequestId.isEmpty())
		return false;
	if (FConfigFormWidget && !FConfigFormWidget->isSubmitValid())
		return false;
	return QWizardPage::isComplete();
}

bool ConfigPage::validatePage()
{
	if (!FRoomConfigured)
	{
		IDataForm form = FConfigFormWidget!=NULL ? FConfigFormWidget->submitDataForm() : IDataForm();
		form.type = DATAFORM_TYPE_SUBMIT;

		FConfigUpdateRequestId = FMultiChat!=NULL ? FMultiChat->updateRoomConfig(form) : QString::null;
		if (!FConfigUpdateRequestId.isEmpty())
		{
			lblInfo->setText(tr("Saving conference settings..."));
			emit completeChanged();
		}
		else
		{
			QMessageBox::warning(this,tr("Error"),tr("Failed to send conference settings"));
		}

		return false;
	}
	return QWizardPage::validatePage();
}

int ConfigPage::nextId() const
{
	return CreateMultiChatWizard::PageJoin;
}

Jid ConfigPage::streamJid() const
{
	return field(WF_ACCOUNT).toString();
}

Jid ConfigPage::roomJid() const
{
	return field(WF_ROOM_JID).toString();
}

QVariant ConfigPage::configHints() const
{
	return FConfigHints;
}

void ConfigPage::setConfigHints(const QVariant &AHints)
{
	FConfigHints = AHints.toMap();
}

void ConfigPage::setError(const QString &AMessage)
{
	prbProgress->setVisible(false);

	if (!FRoomCreated)
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Conference is not created :(")));
	else
		lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Conference is not configured :(")));

	lblInfo->setText(AMessage);
}

void ConfigPage::onConfigFormFieldChanged()
{
	emit completeChanged();
}

void ConfigPage::onMultiChatStateChanged(int AState)
{
	if (AState == IMultiUserChat::Opened)
	{
		FRoomCreated = true;

		FConfigLoadRequestId = FMultiChat->loadRoomConfig();
		if (!FConfigLoadRequestId.isEmpty())
			lblCaption->setText(QString("<h2>%1</h2>").arg(tr("Loading settings...")));
		else
			setError(tr("Failed to load conference settings"));

		emit completeChanged();
	}
	else if (AState == IMultiUserChat::Closed)
	{
		if (!FRoomCreated)
			setError(tr("Failed to create conference: %1").arg(FMultiChat->roomError().errorMessage()));
	}
}

void ConfigPage::onMultiChatConfigLoaded(const QString &AId, const IDataForm &AForm)
{
	if (FConfigLoadRequestId == AId)
	{
		IDataForms *dataForms = PluginHelper::pluginInstance<IDataForms>();
		if (dataForms)
		{
			lblCaption->setVisible(false);
			wdtConfig->setVisible(true);
			prbProgress->setVisible(false);

			lblInfo->setText(QString::null);
			lblInfo->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

			if (FConfigFormWidget)
				delete FConfigFormWidget->instance();

			FConfigFormWidget = dataForms->formWidget(dataForms->localizeForm(AForm), wdtConfig);
			FConfigFormWidget->instance()->layout()->setMargin(0);
			wdtConfig->layout()->addWidget(FConfigFormWidget->instance());

			for (QMap<QString,QVariant>::const_iterator it=FConfigHints.constBegin(); it!=FConfigHints.constEnd(); ++it)
			{
				IDataFieldWidget *field = FConfigFormWidget->fieldWidget(it.key());
				if (field != NULL)
					field->setValue(it.value());
			}
			connect(FConfigFormWidget->instance(),SIGNAL(fieldChanged(IDataFieldWidget *)),SLOT(onConfigFormFieldChanged()));
		}
		else
		{
			setError(tr("Failed to change default conference settings"));
		}

		FConfigLoadRequestId.clear();
		emit completeChanged();
	}
}

void ConfigPage::onMultiChatConfigUpdated(const QString &AId, const IDataForm &AForm)
{
	Q_UNUSED(AForm);
	if (FConfigUpdateRequestId == AId)
	{
		FConfigUpdateRequestId.clear();
		lblInfo->setText(QString::null);

		FRoomConfigured = true;
		wizard()->next();
		FRoomConfigured = false;
	}
}

void ConfigPage::onMultiChatRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FConfigLoadRequestId == AId)
	{
		setError(tr("Failed to load conference settings: %1").arg(AError.errorMessage()));
	}
	else if (FConfigUpdateRequestId == AId)
	{
		setError(tr("Failed to update conference settings: %1").arg(AError.errorMessage()));
	}
	emit completeChanged();
}

/*************
 * JoinPage
 *************/
JoinPage::JoinPage(QWidget *AParent) : QWizardPage(AParent)
{
	setFinalPage(true);
	setButtonText(QWizard::FinishButton, tr("Join"));

	setTitle(tr("Join conference"));
	setSubTitle(tr("Enter parameters to join to the conference"));

	FWaitInfo = false;
	FRoomChecked = false;

	lneNick = new QLineEdit(this);
	lneNick->setPlaceholderText(tr("Nick"));
	connect(lneNick,SIGNAL(textChanged(const QString &)),SLOT(onRoomNickTextChanged()));

	lblRegister = new QLabel(this);
	lblRegister->setTextFormat(Qt::RichText);
	connect(lblRegister,SIGNAL(linkActivated(const QString &)),SLOT(onRegisterNickLinkActivated()));

	lblRoomJid = new QLabel(this);
	lblRoomJid->setWordWrap(true);
	lblRoomJid->setTextFormat(Qt::RichText);

	lblRoomName = new QLabel(this);
	lblRoomName->setWordWrap(true);
	lblRoomName->setTextFormat(Qt::PlainText);

	lnePassword = new QLineEdit(this);
	lnePassword->setVisible(false);
	lnePassword->setEchoMode(QLineEdit::Password);
	lnePassword->setPlaceholderText(tr("Password is required"));
	connect(lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onRoomPasswordTextChanged()));

	lblMucPassword = new QLabel(this);
	lblMucMembersOnly = new QLabel(this);
	lblMucHidden = new QLabel(this);
	lblMucAnonymous = new QLabel(this);
	lblMucModerated = new QLabel(this);
	lblMucTemporary = new QLabel(this);

	lblInfo = new QLabel(this);
	lblInfo->setWordWrap(true);
	lblInfo->setTextFormat(Qt::PlainText);

	QHBoxLayout *nickLayout = new QHBoxLayout;
	nickLayout->addWidget(new QLabel(tr("Join with nick:"),this));
	nickLayout->addWidget(lneNick);
	nickLayout->addWidget(lblRegister);

	QHBoxLayout *passwordLayout = new QHBoxLayout;
	passwordLayout->addWidget(lblMucPassword);
	passwordLayout->addWidget(lnePassword);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addLayout(nickLayout);
	layout->addSpacing(10);
	layout->addWidget(lblRoomJid);
	layout->addWidget(lblRoomName);
	layout->addLayout(passwordLayout);
	layout->addWidget(lblMucMembersOnly);
	layout->addWidget(lblMucAnonymous);
	layout->addWidget(lblMucModerated);
	layout->addWidget(lblMucTemporary);
	layout->addWidget(lblMucHidden);
	layout->addSpacing(10);
	layout->addWidget(lblInfo);
	layout->setMargin(0);

	QWidget::setTabOrder(lneNick,lblRegister);
	QWidget::setTabOrder(lblRegister,lnePassword);

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	if (discovery)
	{
		connect(discovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoRecieved(const IDiscoInfo &)));
	}

	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	if (multiChatManager)
	{
		connect(multiChatManager->instance(),SIGNAL(registeredNickReceived(const QString &, const QString &)),SLOT(onRegisteredNickRecieved(const QString &, const QString &)));
	}

	registerField(WF_ROOM_NICK,this,"roomNick");
	registerField(WF_ROOM_PASSWORD,this,"roomPassword");
}

void JoinPage::initializePage()
{
	FRoomChecked = false;
	processDiscoInfo(IDiscoInfo());

	lblRoomJid->setText(QString("<b>%1</b>").arg(roomJid().uBare()));

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	if (discovery && discovery->requestDiscoInfo(streamJid(),roomJid()))
	{
		FWaitInfo = true;
		lblInfo->setText(tr("Loading conference description..."));
	}

	onRoomNickTextChanged();
	onRegisterNickDialogFinished();
}

bool JoinPage::isComplete() const
{
	if (!FRoomChecked)
		return false;
	if (lneNick->text().trimmed().isEmpty())
		return false;
	if (FRoomInfo.features.contains(MUC_FEATURE_PASSWORD) && lnePassword->text().isEmpty())
		return false;
	if (FRoomInfo.features.contains(MUC_FEATURE_PASSWORDPROTECTED) && lnePassword->text().isEmpty())
		return false;
	return QWizardPage::isComplete();
}

int JoinPage::nextId() const
{
	return -1;
}

int JoinPage::wizardMode() const
{
	return field(WF_MODE).toInt();
}

Jid JoinPage::streamJid() const
{
	return field(WF_ACCOUNT).toString();
}

Jid JoinPage::roomJid() const
{
	return field(WF_ROOM_JID).toString();
}

QString JoinPage::roomNick() const
{
	return lneNick->text();
}

void JoinPage::setRoomNick(const QString &ANick)
{
	lneNick->setText(ANick);
}

QString JoinPage::roomPassword() const
{
	return lnePassword->text();
}

void JoinPage::setRoomPassword(const QString &APassword)
{
	lnePassword->setText(APassword);
}

void JoinPage::processDiscoInfo(const IDiscoInfo &AInfo)
{
	FRoomInfo = AInfo;

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	int index = discovery!=NULL ? discovery->findIdentity(AInfo.identity,DIC_CONFERENCE,DIT_CONFERENCE_TEXT) : -1;
	if (index>=0 && AInfo.error.isNull())
	{
		IDiscoIdentity ident = AInfo.identity.value(index);
		if (!ident.name.isEmpty() && ident.name!=AInfo.contactJid.node())
		{
			lblRoomName->setText(ident.name.trimmed());
			lblRoomName->setVisible(true);
		}
		else
		{
			lblRoomName->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_PASSWORD) || AInfo.features.contains(MUC_FEATURE_PASSWORDPROTECTED))
		{
			lnePassword->setVisible(true);
			lblMucPassword->setVisible(true);
			lblMucPassword->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is password protected")));
			lblMucPassword->setToolTip(discovery->discoFeature(MUC_FEATURE_PASSWORD).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_UNSECURED))
		{
			lnePassword->setVisible(false);
			lblMucPassword->setVisible(true);
			lblMucPassword->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is not password protected")));
			lblMucPassword->setToolTip(discovery->discoFeature(MUC_FEATURE_UNSECURED).description);
		}
		else
		{
			lnePassword->setVisible(false);
			lblMucPassword->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_MEMBERSONLY))
		{
			lblMucMembersOnly->setVisible(true);
			lblMucMembersOnly->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is members only")));
			lblMucMembersOnly->setToolTip(discovery->discoFeature(MUC_FEATURE_MEMBERSONLY).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_OPEN))
		{
			lblMucMembersOnly->setVisible(true);
			lblMucMembersOnly->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is public")));
			lblMucMembersOnly->setToolTip(discovery->discoFeature(MUC_FEATURE_OPEN).description);
		}
		else
		{
			lblMucMembersOnly->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_SEMIANONYMOUS))
		{
			lblMucAnonymous->setVisible(true);
			lblMucAnonymous->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is semi anonymous")));
			lblMucAnonymous->setToolTip(discovery->discoFeature(MUC_FEATURE_SEMIANONYMOUS).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_NONANONYMOUS))
		{
			lblMucAnonymous->setVisible(true);
			lblMucAnonymous->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is not anonymous")));
			lblMucAnonymous->setToolTip(discovery->discoFeature(MUC_FEATURE_NONANONYMOUS).description);
		}
		else
		{
			lblMucAnonymous->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_MODERATED))
		{
			lblMucModerated->setVisible(true);
			lblMucModerated->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is moderated")));
			lblMucModerated->setToolTip(discovery->discoFeature(MUC_FEATURE_MODERATED).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_UNMODERATED))
		{
			lblMucModerated->setVisible(true);
			lblMucModerated->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is not moderated")));
			lblMucModerated->setToolTip(discovery->discoFeature(MUC_FEATURE_UNMODERATED).description);
		}
		else
		{
			lblMucModerated->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_TEMPORARY))
		{
			lblMucTemporary->setVisible(true);
			lblMucTemporary->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is temporary")));
			lblMucTemporary->setToolTip(discovery->discoFeature(MUC_FEATURE_TEMPORARY).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_PERSISTENT))
		{
			lblMucTemporary->setVisible(true);
			lblMucTemporary->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is persistent")));
			lblMucTemporary->setToolTip(discovery->discoFeature(MUC_FEATURE_PERSISTENT).description);
		}
		else
		{
			lblMucTemporary->setVisible(false);
		}

		if (AInfo.features.contains(MUC_FEATURE_HIDDEN))
		{
			lblMucHidden->setVisible(true);
			lblMucHidden->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is hidden")));
			lblMucHidden->setToolTip(discovery->discoFeature(MUC_FEATURE_HIDDEN).description);
		}
		else if (AInfo.features.contains(MUC_FEATURE_PUBLIC))
		{
			lblMucHidden->setVisible(true);
			lblMucHidden->setText(QString("%1 %2").arg(QChar(0x2713)).arg(tr("This conference is visible for all")));
			lblMucHidden->setToolTip(discovery->discoFeature(MUC_FEATURE_PUBLIC).description);
		}
		else
		{
			lblMucHidden->setVisible(false);
		}

		FRoomChecked = true;
		lblInfo->setText(QString::null);
	}
	else
	{
		lblRoomName->setVisible(false);
		lnePassword->setVisible(false);
		lblMucPassword->setVisible(false);
		lblMucMembersOnly->setVisible(false);
		lblMucAnonymous->setVisible(false);
		lblMucModerated->setVisible(false);
		lblMucTemporary->setVisible(false);
		lblMucHidden->setVisible(false);

		if (!AInfo.error.isNull())
			lblInfo->setText(tr("Failed to load conference description: %1").arg(AInfo.error.errorMessage()));
		else
			lblInfo->setText(tr("Conference description is not available or invalid"));
	}
}

void JoinPage::onRoomNickTextChanged()
{
	if (lneNick->text().isEmpty())
	{
		lblRegister->setEnabled(false);
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Register")));
	}
	else if (FRegisteredNick == lneNick->text())
	{
		lblRegister->setEnabled(true);
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Registered")));
	}
	else
	{
		lblRegister->setEnabled(true);
		lblRegister->setText(QString("<a href='register'>%1</a>").arg(tr("Register")));
	}
	emit completeChanged();
}

void JoinPage::onRoomPasswordTextChanged()
{
	emit completeChanged();
}

void JoinPage::onRegisterNickLinkActivated()
{
	if (!lneNick->text().isEmpty() && FRegisteredNick!=lneNick->text())
	{
		IRegistration *registration = PluginHelper::pluginInstance<IRegistration>();
		if (registration)
		{
			QDialog *dialog = registration->showRegisterDialog(streamJid(),roomJid().domain(),IRegistration::Register,this);
			connect(dialog,SIGNAL(finished(int)),SLOT(onRegisterNickDialogFinished()));
			dialog->setWindowModality(Qt::WindowModal);
			dialog->show();
		}
	}
}

void JoinPage::onRegisterNickDialogFinished()
{
	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	FNickRequestId = multiChatManager!=NULL ? multiChatManager->requestRegisteredNick(streamJid(),roomJid()) : QString::null;
	if (!FNickRequestId.isEmpty())
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Loading...")));
	else
		onRegisteredNickRecieved(FNickRequestId, QString::null);
}

void JoinPage::onDiscoInfoRecieved(const IDiscoInfo &AInfo)
{
	if (FWaitInfo && AInfo.streamJid==streamJid() && AInfo.contactJid==roomJid() && AInfo.node.isEmpty())
	{
		FWaitInfo = false;
		processDiscoInfo(AInfo);
	}
}

void JoinPage::onRegisteredNickRecieved(const QString &AId, const QString &ANick)
{
	if (FNickRequestId == AId)
	{
		FRegisteredNick = ANick;
		if (!ANick.isEmpty())
		{
			setRoomNick(ANick);
		}
		else if (lneNick->text().isEmpty())
		{
			QString nick = Options::fileValue(OFV_LAST_NICK).toString();
			if (nick.isEmpty())
			{
				IVCardManager *vcardManager = PluginHelper::pluginInstance<IVCardManager>();
				IVCard *vcard = vcardManager!=NULL ? vcardManager->getVCard(streamJid().bare()) : NULL;
				if (vcard)
				{
					nick = vcard->value(VVN_NICKNAME);
					vcard->unlock();
				}
			}
			setRoomNick(nick.isEmpty() ? streamJid().uNode() : nick);
		}
		onRoomNickTextChanged();
	}
}

/*************
 * JoinPage
 *************/
ManualPage::ManualPage(QWidget *AParent) : QWizardPage(AParent)
{
	setFinalPage(true);
	setButtonText(QWizard::FinishButton, tr("Join"));

	setTitle(tr("Conference parameters"));
	setSubTitle(tr("Enter parameters to join or create the conference"));

	FWaitInfo = false;
	FRoomChecked = false;

	cmbAccount = new QComboBox(this);
	
	lneRoomJid = new QLineEdit(this);
	lneRoomJid->setPlaceholderText(tr("Conference as 'name@service.server.com'"));
	
	lneRoomNick = new QLineEdit(this);
	lneRoomNick->setPlaceholderText(tr("Your nickname in conference"));

	lblRegister = new QLabel(this);
	lblRegister->setTextFormat(Qt::RichText);
	connect(lblRegister,SIGNAL(linkActivated(const QString &)),SLOT(onRegisterNickLinkActivated()));

	lneRoomPassword = new QLineEdit(this);
	lneRoomPassword->setEchoMode(QLineEdit::Password);
	lneRoomPassword->setPlaceholderText(tr("Conference password if required"));

	lblInfo = new QLabel(this);
	lblInfo->setWordWrap(true);
	lblInfo->setTextFormat(Qt::PlainText);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(new QLabel(tr("Account:")),0,0);
	layout->addWidget(cmbAccount,0,1,1,2);
	layout->addWidget(new QLabel(tr("Conference:")),1,0);
	layout->addWidget(lneRoomJid,1,1,1,2);
	layout->addWidget(new QLabel(tr("Nick:")),2,0);
	layout->addWidget(lneRoomNick,2,1);
	layout->addWidget(lblRegister,2,2);
	layout->addWidget(new QLabel(tr("Password:")),3,0);
	layout->addWidget(lneRoomPassword,3,1,1,2);
	layout->addItem(new QSpacerItem(10,10),4,0);
	layout->addWidget(lblInfo,5,0,1,3);
	layout->setMargin(0);

	QWidget::setTabOrder(cmbAccount, lneRoomJid);
	QWidget::setTabOrder(lneRoomJid, lneRoomNick);
	QWidget::setTabOrder(lneRoomNick, lblRegister);
	QWidget::setTabOrder(lblRegister, lneRoomPassword);

	IAccountManager *accountManager = PluginHelper::pluginInstance<IAccountManager>();
	if (accountManager)
	{
		QMap<int, IAccount *> orderedAccounts;
		foreach(IAccount *account, accountManager->accounts())
		{
			if (account->isActive())
				orderedAccounts.insert(account->accountOrder(), account);
		}
		foreach(IAccount *account, orderedAccounts)
		{
			if (account->xmppStream()->isOpen())
				cmbAccount->addItem(account->name(),account->streamJid().pFull());
		}
	}

	IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
	if (discovery)
	{
		connect(discovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoRecieved(const IDiscoInfo &)));
	}

	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	if (multiChatManager)
	{
		connect(multiChatManager->instance(),SIGNAL(registeredNickReceived(const QString &, const QString &)),SLOT(onRegisteredNickRecieved(const QString &, const QString &)));
	}

	FRoomInfoTimer.setSingleShot(true);
	connect(&FRoomInfoTimer,SIGNAL(timeout()),SLOT(onRoomInfoTimerTimeout()));

	connect(cmbAccount,SIGNAL(currentIndexChanged(int)),SLOT(onAccountIndexChanged()));
	connect(lneRoomJid,SIGNAL(textChanged(const QString &)),SLOT(onRoomJidTextChanged()));
	connect(lneRoomNick,SIGNAL(textChanged(const QString &)),SLOT(onRoomNickTextChanged()));

	registerField(WF_MANUAL_ACCOUNT,this,"streamJid");
	registerField(WF_MANUAL_ROOM_JID,this,"roomJid");
	registerField(WF_MANUAL_ROOM_NICK,this,"roomNick");
	registerField(WF_MANUAL_ROOM_PASSWORD,this,"roomPassword");
}

void ManualPage::initializePage()
{
	onAccountIndexChanged();
}

bool ManualPage::isComplete() const
{
	if (streamJid().isEmpty())
		return false;
	if (roomJid().isEmpty())
		return false;
	if (roomNick().isEmpty())
		return false;
	return QWizardPage::isComplete();
}

int ManualPage::nextId() const
{
	return -1;
}

QString ManualPage::streamJid() const
{
	return cmbAccount->itemData(cmbAccount->currentIndex()).toString();
}

void ManualPage::setStreamJid(const QString &AStreamJid)
{
	Jid stream = AStreamJid;
	cmbAccount->setCurrentIndex(cmbAccount->findData(stream.pFull()));
}

QString ManualPage::roomJid() const
{
	Jid room = Jid::fromUserInput(lneRoomJid->text());
	return room.isValid() && room.hasNode() ? room.bare() : QString::null;
}

void ManualPage::setRoomJid(const QString &ARoomJid)
{
	Jid room = ARoomJid;
	lneRoomJid->setText(room.bare());
}

QString ManualPage::roomNick() const
{
	return lneRoomNick->text();
}

void ManualPage::setRoomNick(const QString &ANick)
{
	lneRoomNick->setText(ANick);
}

QString ManualPage::roomPassword() const
{
	return lneRoomPassword->text();
}

void ManualPage::setRoomPassword(const QString &APassword)
{
	lneRoomPassword->setText(APassword);
}

void ManualPage::onAccountIndexChanged()
{
	onRoomJidTextChanged();
}

void ManualPage::onRoomJidTextChanged()
{
	FWaitInfo = false;
	FRoomChecked = false;
	FNickRequestId.clear();
	lblInfo->setText(QString::null);

	FRoomInfoTimer.start(500);
	onRoomNickTextChanged();
}

void ManualPage::onRoomNickTextChanged()
{
	if (!FRoomChecked)
	{
		lblRegister->setEnabled(false);
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Register")));
	}
	else if (FRegisteredNick == lneRoomNick->text())
	{
		lblRegister->setEnabled(true);
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Registered")));
	}
	else
	{
		lblRegister->setEnabled(true);
		lblRegister->setText(QString("<a href='register'>%1</a>").arg(tr("Register")));
	}
	emit completeChanged();
}

void ManualPage::onRoomInfoTimerTimeout()
{
	Jid room = roomJid();
	if (room.isValid() && room.hasNode())
	{
		IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
		if (discovery && discovery->requestDiscoInfo(streamJid(),room))
		{
			FWaitInfo = true;
			lblInfo->setText(tr("Loading conference description..."));
		}
		else
		{
			lblInfo->setText(tr("Failed to load conference description"));
		}
	}
	else if (!room.isEmpty())
	{
		lblInfo->setText(tr("Invalid conference ID"));
	}
}

void ManualPage::onRegisterNickLinkActivated()
{
	IRegistration *registration = PluginHelper::pluginInstance<IRegistration>();
	if (registration)
	{
		Jid room = roomJid();
		QDialog *dialog = registration->showRegisterDialog(streamJid(),room.domain(),IRegistration::Register,this);
		connect(dialog,SIGNAL(finished(int)),SLOT(onRegisterNickDialogFinished()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
	}
}

void ManualPage::onRegisterNickDialogFinished()
{
	IMultiUserChatManager *multiChatManager = PluginHelper::pluginInstance<IMultiUserChatManager>();
	FNickRequestId = multiChatManager!=NULL ? multiChatManager->requestRegisteredNick(streamJid(),roomJid()) : QString::null;
	if (!FNickRequestId.isEmpty())
		lblRegister->setText(QString("<u>%1</u>").arg(tr("Loading...")));
	else
		onRegisteredNickRecieved(FNickRequestId, QString::null);
}

void ManualPage::onDiscoInfoRecieved(const IDiscoInfo &AInfo)
{
	if (FWaitInfo && AInfo.streamJid==streamJid() && AInfo.contactJid==roomJid() && AInfo.node.isEmpty())
	{
		FWaitInfo = false;
		if (AInfo.error.isNull())
		{
			IServiceDiscovery *discovery = PluginHelper::pluginInstance<IServiceDiscovery>();
			int index = discovery!=NULL ? discovery->findIdentity(AInfo.identity,DIC_CONFERENCE,DIT_CONFERENCE_TEXT) : -1;
			if (index >= 0)
			{
				IDiscoIdentity ident = AInfo.identity.value(index);
				lblInfo->setText(!ident.name.isEmpty() ? ident.name.trimmed() : AInfo.contactJid.uNode());

				if (AInfo.features.contains(MUC_FEATURE_PASSWORD) || AInfo.features.contains(MUC_FEATURE_PASSWORDPROTECTED))
					lblInfo->setText(QString("%1\n%2").arg(lblInfo->text(),tr("This conference is password protected")));

				FRoomChecked = true;
			}
			else
			{
				lblInfo->setText(tr("Conference description is not available or invalid"));
			}
		}
		else if (AInfo.error.conditionCode() == XmppStanzaError::EC_ITEM_NOT_FOUND)
		{
			FRoomChecked = true;
			lblInfo->setText(tr("This conference does not exists and will be automatically created on join"));
		}
		else if (AInfo.error.conditionCode() == XmppStanzaError::EC_REMOTE_SERVER_NOT_FOUND)
		{
			Jid room = roomJid();
			lblInfo->setText(tr("Conference service '%1' is not available or does not exists").arg(room.domain()));
		}
		else
		{
			lblInfo->setText(tr("Failed to check conference existence: %1").arg(AInfo.error.errorMessage()));
		}

		if (FRoomChecked)
			onRegisterNickDialogFinished();
		else
			onRoomNickTextChanged();
	}
}

void ManualPage::onRegisteredNickRecieved(const QString &AId, const QString &ANick)
{
	if (FNickRequestId == AId)
	{
		FRegisteredNick = ANick;
		if (!ANick.isEmpty())
		{
			setRoomNick(ANick);
		}
		else if (lneRoomNick->text().isEmpty())
		{
			Jid stream = streamJid();
			QString nick = Options::fileValue(OFV_LAST_NICK).toString();
			if (nick.isEmpty())
			{
				IVCardManager *vcardManager = PluginHelper::pluginInstance<IVCardManager>();
				IVCard *vcard = vcardManager!=NULL ? vcardManager->getVCard(stream.bare()) : NULL;
				if (vcard)
				{
					nick = vcard->value(VVN_NICKNAME);
					vcard->unlock();
				}
			}
			setRoomNick(nick.isEmpty() ? stream.uNode() : nick);
		}
		onRoomNickTextChanged();
	}
}
