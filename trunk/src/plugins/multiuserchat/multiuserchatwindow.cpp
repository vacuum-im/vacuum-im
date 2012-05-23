#include "multiuserchatwindow.h"

#include <QTimer>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QInputDialog>
#include <QCoreApplication>
#include <QContextMenuEvent>

#define ADR_STREAM_JID              Action::DR_StreamJid
#define ADR_ROOM_JID                Action::DR_Parametr1
#define ADR_USER_JID                Action::DR_Parametr2
#define ADR_USER_REAL_JID           Action::DR_Parametr3
#define ADR_USER_NICK               Action::DR_Parametr4
#define ADR_USER_ROLE               Action::DR_UserDefined + 1
#define ADR_USER_AFFIL              Action::DR_UserDefined + 2

#define NICK_MENU_KEY               Qt::Key_Tab

#define HISTORY_MESSAGES            10
#define HISTORY_TIME_DELTA          5
#define HISTORY_DUBLICATE_DELTA     2*60

#define REJOIN_AFTER_KICK_MSEC      500

MultiUserChatWindow::MultiUserChatWindow(IMultiUserChatPlugin *AChatPlugin, IMultiUserChat *AMultiChat)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);

	FStatusIcons = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FStatusChanger = NULL;
	FMessageArchiver = NULL;

	FMultiChat = AMultiChat;
	FChatPlugin = AChatPlugin;
	FMultiChat->instance()->setParent(this);

	FViewWidget = NULL;
	FEditWidget = NULL;
	FMenuBarWidget = NULL;
	FToolBarWidget = NULL;
	FStatusBarWidget = NULL;
	FTabPageNotifier = NULL;
	FShownDetached = false;
	FDestroyOnChatClosed = false;
	FUsersListWidth = -1;

	FStartCompletePos = 0;
	FCompleteIt = FCompleteNicks.constEnd();

	initialize();
	createMessageWidgets();
	setMessageStyle();
	connectMultiChat();
	createStaticRoomActions();
	updateStaticRoomActions();
	loadWindowState();

	FUsersModel = new QStandardItemModel(0,1,ui.ltvUsers);

	FUsersProxy = new UsersProxyModel(AMultiChat,FUsersModel);
	FUsersProxy->setSortLocaleAware(true);
	FUsersProxy->setDynamicSortFilter(true);
	FUsersProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
	FUsersProxy->setSourceModel(FUsersModel);
	FUsersProxy->sort(0, Qt::AscendingOrder);

	ui.ltvUsers->setModel(FUsersProxy);
	ui.ltvUsers->viewport()->installEventFilter(this);
	connect(ui.ltvUsers,SIGNAL(doubleClicked(const QModelIndex &)),SLOT(onUserItemDoubleClicked(const QModelIndex &)));

	ui.sprHSplitter->installEventFilter(this);
	connect(ui.sprHSplitter,SIGNAL(splitterMoved(int,int)),SLOT(onHorizontalSplitterMoved(int,int)));

	connect(this,SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));

	updateWindow();
}

MultiUserChatWindow::~MultiUserChatWindow()
{
	QList<IChatWindow *> chatWindows = FChatWindows;
	foreach(IChatWindow *window,chatWindows)
		delete window->instance();

	if (FMessageProcessor)
		FMessageProcessor->removeMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);

	saveWindowState();
	emit tabPageDestroyed();
}

QString MultiUserChatWindow::tabPageId() const
{
	return "MessageWindow|"+streamJid().pBare()+"|"+roomJid().pBare();
}

bool MultiUserChatWindow::isVisibleTabPage() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return widget->isVisible();
}

bool MultiUserChatWindow::isActiveTabPage() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

void MultiUserChatWindow::assignTabPage()
{
	if (FMessageWidgets && isWindow() && !QMainWindow::isVisible())
		FMessageWidgets->assignTabWindowPage(this);
	else
		emit tabPageAssign();
}

void MultiUserChatWindow::showTabPage()
{
	assignTabPage();
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void MultiUserChatWindow::showMinimizedTabPage()
{
	assignTabPage();
	if (isWindow() && !isVisible())
		showMinimized();
	else
		emit tabPageShowMinimized();
}

void MultiUserChatWindow::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QIcon MultiUserChatWindow::tabPageIcon() const
{
	return windowIcon();
}

QString MultiUserChatWindow::tabPageCaption() const
{
	return windowIconText();
}

QString MultiUserChatWindow::tabPageToolTip() const
{
	return FTabPageToolTip;
}

ITabPageNotifier *MultiUserChatWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void MultiUserChatWindow::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

bool MultiUserChatWindow::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder);
	if (ADirection == IMessageProcessor::MessageIn)
		return (streamJid() == AMessage.to()) && (roomJid() && AMessage.from());
	else
		return (streamJid() == AMessage.from()) && (roomJid() && AMessage.to());
}

bool MultiUserChatWindow::messageDisplay(const Message &AMessage, int ADirection)
{
	bool displayed = false;
	if (AMessage.type() != Message::Error)
	{
		if (ADirection == IMessageProcessor::MessageIn)
		{
			Jid contactJid = AMessage.from();
			if (contactJid.resource().isEmpty() && !AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
			{
				displayed = true;
				IDataForm form = FDataForms->dataForm(AMessage.stanza().firstElement("x",NS_JABBER_DATA));
				IDataDialogWidget *dialog = FDataForms->dialogWidget(form,this);
				connect(dialog->instance(),SIGNAL(accepted()),SLOT(onDataFormMessageDialogAccepted()));
				showStatusMessage(tr("Data form received: %1").arg(form.title),IMessageContentOptions::TypeNotification);
				FDataFormMessages.insert(AMessage.data(MDR_MESSAGE_ID).toInt(),dialog);
			}
			else if (AMessage.type()==Message::GroupChat || contactJid.resource().isEmpty())
			{
				if (!AMessage.body().isEmpty())
				{
					displayed = true;
					if (FHistoryRequests.values().contains(NULL))
						FPendingMessages[NULL].append(AMessage);
					showUserMessage(AMessage,contactJid.resource());
				}
			}
			else if (!AMessage.body().isEmpty())
			{
				IChatWindow *window = getChatWindow(contactJid);
				if (window)
				{
					displayed = true;
					if (FHistoryRequests.values().contains(window))
						FPendingMessages[window].append(AMessage);
					showChatMessage(window,AMessage);
				}
			}
		}
		else if (ADirection == IMessageProcessor::MessageOut)
		{
			Jid contactJid = AMessage.to();
			if (!contactJid.resource().isEmpty() && !AMessage.body().isEmpty())
			{
				IChatWindow *window = getChatWindow(contactJid);
				if (window)
				{
					displayed = true;
					if (FHistoryRequests.values().contains(window))
						FPendingMessages[window].append(AMessage);
					showChatMessage(window,AMessage);
				}
			}
		}
	}
	return displayed;
}

INotification MultiUserChatWindow::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection==IMessageProcessor::MessageIn && AMessage.type()!=Message::Error)
	{
		Jid contactJid = AMessage.from();
		int messageId = AMessage.data(MDR_MESSAGE_ID).toInt();
		IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
		if (!contactJid.resource().isEmpty())
		{
			ITabPage *page = NULL;
			if (AMessage.type() == Message::GroupChat)
			{
				if (!AMessage.body().isEmpty() && !isActiveTabPage() && !AMessage.isDelayed())
				{
					page = this;
					if (isMentionMessage(AMessage))
					{
						notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_MENTION);
						notify.typeId = NNT_MUC_MESSAGE_MENTION;
						notify.data.insert(NDR_TOOLTIP,tr("Mention message in conference: %1").arg(contactJid.uNode()));
						notify.data.insert(NDR_POPUP_CAPTION,tr("Mention in conference"));
					}
					else
					{
						notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_GROUPCHAT);
						notify.typeId = NNT_MUC_MESSAGE_GROUPCHAT;
						notify.data.insert(NDR_TOOLTIP,tr("New message in conference: %1").arg(contactJid.uNode()));
						notify.data.insert(NDR_POPUP_CAPTION,tr("Conference message"));
					}
					notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_MESSAGE));
					notify.data.insert(NDR_POPUP_TITLE,tr("[%1] in conference %2").arg(contactJid.resource()).arg(contactJid.uNode()));
					notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);

					FActiveMessages.append(messageId);
				}
			}
			else if (!AMessage.body().isEmpty())
			{
				IChatWindow *window = getChatWindow(AMessage.from());
				if (window && !window->isActiveTabPage())
				{
					page = window;
					notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_PRIVATE);
					notify.typeId = NNT_MUC_MESSAGE_PRIVATE;
					notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_PRIVATE_MESSAGE));
					notify.data.insert(NDR_TOOLTIP,tr("Private message from: [%1]").arg(contactJid.resource()));
					notify.data.insert(NDR_POPUP_CAPTION,tr("Private message"));
					notify.data.insert(NDR_POPUP_TITLE,tr("[%1] in conference %2").arg(contactJid.resource()).arg(contactJid.uNode()));
					notify.data.insert(NDR_SOUND_FILE,SDF_MUC_PRIVATE_MESSAGE);

					if (FDestroyTimers.contains(window))
						delete FDestroyTimers.take(window);
					FActiveChatMessages.insertMulti(window, messageId);
					updateListItem(contactJid);
				}
			}
			if (notify.kinds & INotification::PopupWindow)
			{
				if (FMessageProcessor)
				{
					QTextDocument doc;
					FMessageProcessor->messageToText(&doc,AMessage);
					notify.data.insert(NDR_POPUP_HTML,TextManager::getDocumentBody(doc));
				}
				else
				{
					notify.data.insert(NDR_POPUP_HTML,Qt::escape(AMessage.body()));
				}
			}
			if (page)
			{
				notify.data.insert(NDR_ALERT_WIDGET,(qint64)page->instance());
				notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)page->instance());
				notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_NEW_MESSAGE);
				notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)page->instance());
			}
		}
		else if (!AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
		{
			IDataDialogWidget *dialog = FDataFormMessages.value(messageId);
			if (dialog && !dialog->instance()->isActiveWindow())
			{
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_PRIVATE);
				notify.typeId = NNT_MUC_MESSAGE_PRIVATE;
				notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_DATA_MESSAGE));
				notify.data.insert(NDR_TOOLTIP,tr("Data form received from: %1").arg(contactJid.uNode()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Data form received"));
				notify.data.insert(NDR_POPUP_TITLE,ANotifications->contactName(FMultiChat->streamJid(),contactJid));
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(contactJid));
				notify.data.insert(NDR_POPUP_HTML,Qt::escape(AMessage.stanza().firstElement("x",NS_JABBER_DATA).firstChildElement("instructions").text()));
				notify.data.insert(NDR_SOUND_FILE,SDF_MUC_DATA_MESSAGE);
				notify.data.insert(NDR_ALERT_WIDGET,(qint64)dialog->instance());
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)dialog->instance());
			}
		}
	}
	return notify;
}

bool MultiUserChatWindow::messageShowWindow(int AMessageId)
{
	if (FActiveMessages.contains(AMessageId))
	{
		showTabPage();
	}
	else if (FActiveChatMessages.values().contains(AMessageId))
	{
		IChatWindow *window = FActiveChatMessages.key(AMessageId);
		if (window)
		{
			window->showTabPage();
			return true;
		}
	}
	else if (FDataFormMessages.contains(AMessageId))
	{
		IDataDialogWidget *dialog = FDataFormMessages.take(AMessageId);
		if (dialog)
		{
			dialog->instance()->show();
			FMessageProcessor->removeMessageNotify(AMessageId);
			return true;
		}
	}
	return false;
}

bool MultiUserChatWindow::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if ((streamJid() == AStreamJid) && (roomJid() && AContactJid))
	{
		if (AType == Message::GroupChat)
		{
			if (AShowMode == IMessageHandler::SM_ASSIGN)
				assignTabPage();
			else if (AShowMode == IMessageHandler::SM_SHOW)
				showTabPage();
			else if (AShowMode == IMessageHandler::SM_MINIMIZED)
				showMinimizedTabPage();
		}
		else
		{
			IChatWindow *window = getChatWindow(AContactJid);
			if (window)
			{
				if (AShowMode == IMessageHandler::SM_ASSIGN)
					window->assignTabPage();
				else if (AShowMode == IMessageHandler::SM_SHOW)
					window->showTabPage();
				else if (AShowMode == IMessageHandler::SM_MINIMIZED)
					window->showMinimizedTabPage();
			}
		}
		return true;
	}
	return false;
}

Jid MultiUserChatWindow::streamJid() const
{
	return FMultiChat->streamJid();
}

Jid MultiUserChatWindow::roomJid() const
{
	return FMultiChat->roomJid();
}

IViewWidget *MultiUserChatWindow::viewWidget() const
{
	return FViewWidget;
}

IEditWidget *MultiUserChatWindow::editWidget() const
{
	return FEditWidget;
}

IMenuBarWidget *MultiUserChatWindow::menuBarWidget() const
{
	return FMenuBarWidget;
}

IToolBarWidget *MultiUserChatWindow::toolBarWidget() const
{
	return FToolBarWidget;
}

IStatusBarWidget *MultiUserChatWindow::statusBarWidget() const
{
	return FStatusBarWidget;
}

IMultiUserChat *MultiUserChatWindow::multiUserChat() const
{
	return FMultiChat;
}

IChatWindow *MultiUserChatWindow::openChatWindow(const Jid &AContactJid)
{
	messageShowWindow(MHO_MULTIUSERCHAT_GROUPCHAT,streamJid(),AContactJid,Message::Chat,IMessageHandler::SM_SHOW);
	return findChatWindow(AContactJid);
}

IChatWindow *MultiUserChatWindow::findChatWindow(const Jid &AContactJid) const
{
	foreach(IChatWindow *window,FChatWindows)
		if (window->contactJid() == AContactJid)
			return window;
	return NULL;
}

void MultiUserChatWindow::contextMenuForUser(IMultiUser *AUser, Menu *AMenu)
{
	if (AMenu && FMultiChat->isOpen() && FUsers.contains(AUser) && AUser!=FMultiChat->mainUser())
	{
		Action *openChat = new Action(AMenu);
		openChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_PRIVATE_MESSAGE);
		openChat->setText(tr("Open chat dialog"));
		openChat->setData(ADR_USER_NICK,AUser->nickName());
		connect(openChat,SIGNAL(triggered(bool)),SLOT(onOpenChatWindowActionTriggered(bool)));
		AMenu->addAction(openChat,AG_MUCM_MULTIUSERCHAT_PRIVATE,true);

		if (FMultiChat->mainUser()->role() == MUC_ROLE_MODERATOR)
		{
			Action *setRoleNone = new Action(AMenu);
			setRoleNone->setText(tr("Kick user"));
			setRoleNone->setData(ADR_USER_NICK,AUser->nickName());
			setRoleNone->setData(ADR_USER_ROLE,MUC_ROLE_NONE);
			connect(setRoleNone,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
			AMenu->addAction(setRoleNone,AG_MUCM_MULTIUSERCHAT_UTILS,false);

			Action *setAffilOutcast = new Action(AMenu);
			setAffilOutcast->setText(tr("Ban user"));
			setAffilOutcast->setData(ADR_USER_NICK,AUser->nickName());
			setAffilOutcast->setData(ADR_USER_AFFIL,MUC_AFFIL_OUTCAST);
			connect(setAffilOutcast,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
			AMenu->addAction(setAffilOutcast,AG_MUCM_MULTIUSERCHAT_UTILS,false);

			Menu *changeRole = new Menu(AMenu);
			changeRole->setTitle(tr("Change Role"));
			{
				Action *setRoleVisitor = new Action(changeRole);
				setRoleVisitor->setCheckable(true);
				setRoleVisitor->setText(tr("Visitor"));
				setRoleVisitor->setData(ADR_USER_NICK,AUser->nickName());
				setRoleVisitor->setData(ADR_USER_ROLE,MUC_ROLE_VISITOR);
				setRoleVisitor->setChecked(AUser->role() == MUC_ROLE_VISITOR);
				connect(setRoleVisitor,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
				changeRole->addAction(setRoleVisitor,AG_DEFAULT,false);

				Action *setRoleParticipant = new Action(changeRole);
				setRoleParticipant->setCheckable(true);
				setRoleParticipant->setText(tr("Participant"));
				setRoleParticipant->setData(ADR_USER_NICK,AUser->nickName());
				setRoleParticipant->setData(ADR_USER_ROLE,MUC_ROLE_PARTICIPANT);
				setRoleParticipant->setChecked(AUser->role() == MUC_ROLE_PARTICIPANT);
				connect(setRoleParticipant,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
				changeRole->addAction(setRoleParticipant,AG_DEFAULT,false);

				Action *setRoleModerator = new Action(changeRole);
				setRoleModerator->setCheckable(true);
				setRoleModerator->setText(tr("Moderator"));
				setRoleModerator->setData(ADR_USER_NICK,AUser->nickName());
				setRoleModerator->setData(ADR_USER_ROLE,MUC_ROLE_MODERATOR);
				setRoleModerator->setChecked(AUser->role() == MUC_ROLE_MODERATOR);
				connect(setRoleModerator,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
				changeRole->addAction(setRoleModerator,AG_DEFAULT,false);
			}
			AMenu->addAction(changeRole->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);

			Menu *changeAffiliation = new Menu(AMenu);
			changeAffiliation->setTitle(tr("Change Affiliation"));
			{
				Action *setAffilNone = new Action(changeAffiliation);
				setAffilNone->setCheckable(true);
				setAffilNone->setText(tr("None"));
				setAffilNone->setData(ADR_USER_NICK,AUser->nickName());
				setAffilNone->setData(ADR_USER_AFFIL,MUC_AFFIL_NONE);
				setAffilNone->setChecked(AUser->affiliation() == MUC_AFFIL_NONE);
				connect(setAffilNone,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
				changeAffiliation->addAction(setAffilNone,AG_DEFAULT,false);

				Action *setAffilMember = new Action(changeAffiliation);
				setAffilMember->setCheckable(true);
				setAffilMember->setText(tr("Member"));
				setAffilMember->setData(ADR_USER_NICK,AUser->nickName());
				setAffilMember->setData(ADR_USER_AFFIL,MUC_AFFIL_MEMBER);
				setAffilMember->setChecked(AUser->affiliation() == MUC_AFFIL_MEMBER);
				connect(setAffilMember,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
				changeAffiliation->addAction(setAffilMember,AG_DEFAULT,false);

				Action *setAffilAdmin = new Action(changeAffiliation);
				setAffilAdmin->setCheckable(true);
				setAffilAdmin->setText(tr("Administrator"));
				setAffilAdmin->setData(ADR_USER_NICK,AUser->nickName());
				setAffilAdmin->setData(ADR_USER_AFFIL,MUC_AFFIL_ADMIN);
				setAffilAdmin->setChecked(AUser->affiliation() == MUC_AFFIL_ADMIN);
				connect(setAffilAdmin,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
				changeAffiliation->addAction(setAffilAdmin,AG_DEFAULT,false);

				Action *setAffilOwner = new Action(changeAffiliation);
				setAffilOwner->setCheckable(true);
				setAffilOwner->setText(tr("Owner"));
				setAffilOwner->setData(ADR_USER_NICK,AUser->nickName());
				setAffilOwner->setData(ADR_USER_AFFIL,MUC_AFFIL_OWNER);
				setAffilOwner->setChecked(AUser->affiliation() == MUC_AFFIL_OWNER);
				connect(setAffilOwner,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
				changeAffiliation->addAction(setAffilOwner,AG_DEFAULT,false);
			}
			AMenu->addAction(changeAffiliation->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);
		}

		emit multiUserContextMenu(AUser,AMenu);
	}
}

void MultiUserChatWindow::exitAndDestroy(const QString &AStatus, int AWaitClose)
{
	closeTabPage();

	FDestroyOnChatClosed = true;
	if (FMultiChat->isOpen())
		FMultiChat->setPresence(IPresence::Offline,AStatus);

	if (AWaitClose>0)
		QTimer::singleShot(FMultiChat->isOpen() ? AWaitClose : 0, this, SLOT(deleteLater()));
	else
		delete this;
}

void MultiUserChatWindow::initialize()
{
	IPlugin *plugin = FChatPlugin->pluginManager()->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = FChatPlugin->pluginManager()->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		IAccountManager *accountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (accountManager)
		{
			IAccount *account = accountManager->accountByStream(streamJid());
			if (account)
			{
				ui.lblAccount->setText(account->name());
				connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),
					SLOT(onAccountOptionsChanged(const OptionsNode &)));
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
			FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
		if (FMessageArchiver)
		{
			connect(FMessageArchiver->instance(),SIGNAL(messagesLoaded(const QString &, const IArchiveCollectionBody &)),
				SLOT(onArchiveMessagesLoaded(const QString &, const IArchiveCollectionBody &)));
			connect(FMessageArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
				SLOT(onArchiveRequestFailed(const QString &, const QString &)));
		}
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));
}

void MultiUserChatWindow::connectMultiChat()
{
	connect(FMultiChat->instance(),SIGNAL(chatOpened()),SLOT(onChatOpened()));
	connect(FMultiChat->instance(),SIGNAL(chatNotify(const QString &)),SLOT(onChatNotify(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(chatError(const QString &)), SLOT(onChatError(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(chatClosed()),SLOT(onChatClosed()));
	connect(FMultiChat->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),
		SLOT(onStreamJidChanged(const Jid &, const Jid &)));
	connect(FMultiChat->instance(),SIGNAL(userPresence(IMultiUser *,int,const QString &)),
		SLOT(onUserPresence(IMultiUser *,int,const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userDataChanged(IMultiUser *, int, const QVariant, const QVariant &)),
		SLOT(onUserDataChanged(IMultiUser *, int, const QVariant, const QVariant &)));
	connect(FMultiChat->instance(),SIGNAL(userNickChanged(IMultiUser *, const QString &, const QString &)),
		SLOT(onUserNickChanged(IMultiUser *, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(presenceChanged(int, const QString &)),
		SLOT(onPresenceChanged(int, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(subjectChanged(const QString &, const QString &)),
		SLOT(onSubjectChanged(const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(serviceMessageReceived(const Message &)),
		SLOT(onServiceMessageReceived(const Message &)));
	connect(FMultiChat->instance(),SIGNAL(inviteDeclined(const Jid &, const QString &)),
		SLOT(onInviteDeclined(const Jid &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userKicked(const QString &, const QString &, const QString &)),
		SLOT(onUserKicked(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userBanned(const QString &, const QString &, const QString &)),
		SLOT(onUserBanned(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(affiliationListReceived(const QString &,const QList<IMultiUserListItem> &)),
		SLOT(onAffiliationListReceived(const QString &,const QList<IMultiUserListItem> &)));
	connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)), SLOT(onConfigFormReceived(const IDataForm &)));
	connect(FMultiChat->instance(),SIGNAL(roomDestroyed(const QString &)), SLOT(onRoomDestroyed(const QString &)));
}

void MultiUserChatWindow::createMessageWidgets()
{
	if (FMessageWidgets)
	{
		ui.wdtView->setLayout(new QVBoxLayout);
		ui.wdtView->layout()->setMargin(0);
		FViewWidget = FMessageWidgets->newViewWidget(FMultiChat->streamJid(),FMultiChat->roomJid(),ui.wdtView);
		connect(FViewWidget->instance(),SIGNAL(viewContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)),
			SLOT(onViewWidgetContextMenu(const QPoint &, const QTextDocumentFragment &, Menu *)));
		ui.wdtView->layout()->addWidget(FViewWidget->instance());
		FWindowStatus[FViewWidget].createTime = QDateTime::currentDateTime();

		ui.wdtEdit->setLayout(new QVBoxLayout);
		ui.wdtEdit->layout()->setMargin(0);
		FEditWidget = FMessageWidgets->newEditWidget(FMultiChat->streamJid(),FMultiChat->roomJid(),ui.wdtEdit);
		FEditWidget->setSendShortcut(SCT_MESSAGEWINDOWS_MUC_SENDMESSAGE);
		ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
		connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
		connect(FEditWidget->instance(),SIGNAL(messageAboutToBeSend()),SLOT(onMessageAboutToBeSend()));
		connect(FEditWidget->instance(),SIGNAL(keyEventReceived(QKeyEvent *,bool &)),SLOT(onEditWidgetKeyEvent(QKeyEvent *,bool &)));

		ui.wdtToolBar->setLayout(new QVBoxLayout);
		ui.wdtToolBar->layout()->setMargin(0);
		FToolBarWidget = FMessageWidgets->newToolBarWidget(NULL,FViewWidget,FEditWidget,NULL,ui.wdtToolBar);
		ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());
		FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

		FMenuBarWidget = FMessageWidgets->newMenuBarWidget(NULL,FViewWidget,FEditWidget,NULL,this);
		setMenuBar(FMenuBarWidget->instance());

		FStatusBarWidget = FMessageWidgets->newStatusBarWidget(NULL,FViewWidget,FEditWidget,NULL,this);
		setStatusBar(FStatusBarWidget->instance());

		setTabPageNotifier(FMessageWidgets->newTabPageNotifier(this));
		connect(tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onNotifierActiveNotifyChanged(int)));
	}
}

void MultiUserChatWindow::createStaticRoomActions()
{
	FToolsMenu = new Menu(FToolBarWidget->toolBarChanger()->toolBar());
	FToolsMenu->setTitle(tr("Tools"));
	FToolsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_MUC_TOOLS_MENU);
	QToolButton *toolsButton = FToolBarWidget->toolBarChanger()->insertAction(FToolsMenu->menuAction(), TBG_MCWTBW_ROOM_TOOLS);
	toolsButton->setPopupMode(QToolButton::InstantPopup);

	FChangeNick = new Action(FToolsMenu);
	FChangeNick->setText(tr("Change room nick"));
	FChangeNick->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_NICK);
	FChangeNick->setShortcutId(SCT_MESSAGEWINDOWS_MUC_CHANGENICK);
	connect(FChangeNick,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FChangeNick,AG_MUTM_MULTIUSERCHAT_COMMON,false);

	FInviteContact = new Action(FToolsMenu);
	FInviteContact->setText(tr("Invite to this room"));
	FInviteContact->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
	connect(FInviteContact,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FInviteContact,AG_MUTM_MULTIUSERCHAT_COMMON,false);

	FRequestVoice = new Action(FToolsMenu);
	FRequestVoice->setText(tr("Request voice"));
	FRequestVoice->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_REQUEST_VOICE);
	connect(FRequestVoice,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FRequestVoice,AG_MUTM_MULTIUSERCHAT_COMMON,false);

	FClearChat = new Action(FToolsMenu);
	FClearChat->setText(tr("Clear chat window"));
	FClearChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
	FClearChat->setShortcutId(SCT_MESSAGEWINDOWS_MUC_CLEARWINDOW);
	connect(FClearChat,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolBarWidget->toolBarChanger()->insertAction(FClearChat,TBG_MWTBW_CLEAR_WINDOW);

	FChangeSubject = new Action(FToolsMenu);
	FChangeSubject->setText(tr("Change topic"));
	FChangeSubject->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_TOPIC);
	FChangeSubject->setShortcutId(SCT_MESSAGEWINDOWS_MUC_CHANGETOPIC);
	connect(FChangeSubject,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FChangeSubject,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FBanList = new Action(FToolsMenu);
	FBanList->setText(tr("Edit ban list"));
	FBanList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_BAN_LIST);
	connect(FBanList,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FBanList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FMembersList = new Action(FToolsMenu);
	FMembersList->setText(tr("Edit members list"));
	FMembersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_MEMBERS_LIST);
	connect(FMembersList,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FMembersList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FAdminsList = new Action(FToolsMenu);
	FAdminsList->setText(tr("Edit administrators list"));
	FAdminsList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_ADMINS_LIST);
	connect(FAdminsList,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FAdminsList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FOwnersList = new Action(FToolsMenu);
	FOwnersList->setText(tr("Edit owners list"));
	FOwnersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_OWNERS_LIST);
	connect(FOwnersList,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FOwnersList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FConfigRoom = new Action(FToolsMenu);
	FConfigRoom->setText(tr("Configure room"));
	FConfigRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFIGURE_ROOM);
	FConfigRoom->setShortcutId(SCT_MESSAGEWINDOWS_MUC_ROOMSETTINGS);
	connect(FConfigRoom,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FConfigRoom,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FDestroyRoom = new Action(FToolsMenu);
	FDestroyRoom->setText(tr("Destroy room"));
	FDestroyRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_DESTROY_ROOM);
	connect(FDestroyRoom,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolsMenu->addAction(FDestroyRoom,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

	FEnterRoom = new Action(FToolsMenu);
	FEnterRoom->setText(tr("Enter room"));
	FEnterRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
	FEnterRoom->setShortcutId(SCT_MESSAGEWINDOWS_MUC_ENTER);
	connect(FEnterRoom,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FExitRoom = new Action(FToolBarWidget->toolBarChanger()->toolBar());
	FExitRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
	FExitRoom->setText(tr("Exit room"));
	FExitRoom->setShortcutId(SCT_MESSAGEWINDOWS_MUC_EXIT);
	connect(FExitRoom,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
	FToolBarWidget->toolBarChanger()->insertAction(FExitRoom, TBG_MCWTBW_ROOM_EXIT);
}

void MultiUserChatWindow::updateStaticRoomActions()
{
	QString role = FMultiChat->isOpen() ? FMultiChat->mainUser()->role() : MUC_ROLE_NONE;
	QString affiliation = FMultiChat->isOpen() ? FMultiChat->mainUser()->affiliation() : MUC_AFFIL_NONE;
	if (affiliation == MUC_AFFIL_OWNER)
	{
		FChangeSubject->setVisible(true);
		FInviteContact->setVisible(true);
		FRequestVoice->setVisible(false);
		FBanList->setVisible(true);
		FMembersList->setVisible(true);
		FAdminsList->setVisible(true);
		FOwnersList->setVisible(true);
		FConfigRoom->setVisible(true);
		FDestroyRoom->setVisible(true);
	}
	else if (affiliation == MUC_AFFIL_ADMIN)
	{
		FChangeSubject->setVisible(true);
		FInviteContact->setVisible(true);
		FRequestVoice->setVisible(false);
		FBanList->setVisible(true);
		FMembersList->setVisible(true);
		FAdminsList->setVisible(true);
		FOwnersList->setVisible(true);
		FConfigRoom->setVisible(false);
		FDestroyRoom->setVisible(false);
	}
	else if (role == MUC_ROLE_VISITOR)
	{
		FChangeSubject->setVisible(false);
		FInviteContact->setVisible(false);
		FRequestVoice->setVisible(true);
		FBanList->setVisible(false);
		FMembersList->setVisible(false);
		FAdminsList->setVisible(false);
		FOwnersList->setVisible(false);
		FConfigRoom->setVisible(false);
		FDestroyRoom->setVisible(false);
	}
	else
	{
		FChangeSubject->setVisible(true);
		FInviteContact->setVisible(true);
		FRequestVoice->setVisible(false);
		FBanList->setVisible(false);
		FMembersList->setVisible(false);
		FAdminsList->setVisible(false);
		FOwnersList->setVisible(false);
		FConfigRoom->setVisible(false);
		FDestroyRoom->setVisible(false);
	}

	if (!FMultiChat->isOpen())
	{
		if (FToolBarWidget->toolBarChanger()->actionHandle(FEnterRoom)==NULL)
			FToolBarWidget->toolBarChanger()->insertAction(FEnterRoom, TBG_MCWTBW_ROOM_ENTER);
	}
	else
	{
		FToolBarWidget->toolBarChanger()->removeItem(FToolBarWidget->toolBarChanger()->actionHandle(FEnterRoom));
	}
}

void MultiUserChatWindow::saveWindowState()
{
	if (FUsersListWidth > 0)
		Options::setFileValue(FUsersListWidth,"muc.mucwindow.users-list-width",tabPageId());
}

void MultiUserChatWindow::loadWindowState()
{
	FUsersListWidth = Options::fileValue("muc.mucwindow.users-list-width",tabPageId()).toInt();
}

void MultiUserChatWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"muc.mucwindow.state",tabPageId());
		Options::setFileValue(saveGeometry(),"muc.mucwindow.geometry",tabPageId());
	}
}

void MultiUserChatWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		if (!restoreGeometry(Options::fileValue("muc.mucwindow.geometry",tabPageId()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("muc.mucwindow.state",tabPageId()).toByteArray());
	}
}

void MultiUserChatWindow::showDateSeparator(IViewWidget *AView, const QDateTime &ADateTime)
{
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
	{
		QDate sepDate = ADateTime.date();
		WindowStatus &wstatus = FWindowStatus[AView];
		if (FMessageStyles && sepDate.isValid() && wstatus.lastDateSeparator!=sepDate)
		{
			IMessageContentOptions options;
			options.kind = IMessageContentOptions::KindStatus;
			if (wstatus.createTime > ADateTime)
				options.type |= IMessageContentOptions::TypeHistory;
			options.status = IMessageContentOptions::StatusDateSeparator;
			options.direction = IMessageContentOptions::DirectionIn;
			options.time.setDate(sepDate);
			options.time.setTime(QTime(0,0));
			options.timeFormat = " ";
			wstatus.lastDateSeparator = sepDate;
			AView->appendText(FMessageStyles->dateSeparator(sepDate),options);
		}
	}
}

bool MultiUserChatWindow::showStatusCodes(const QString &ANick, const QList<int> &ACodes)
{
	bool shown = false;
	if (!ACodes.isEmpty())
	{
		if (ACodes.contains(MUC_SC_NON_ANONYMOUS))
		{
			showStatusMessage(tr("Any occupant is allowed to see the user's full JID"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_AFFIL_CHANGED))
		{
			showStatusMessage(tr("%1 affiliation changed while not in the room").arg(ANick),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_CONFIG_CHANGED))
		{
			showStatusMessage(tr("Room configuration change has occurred"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NOW_LOGGING_ENABLED))
		{
			showStatusMessage(tr("Room logging is now enabled"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NOW_LOGGING_DISABLED))
		{
			showStatusMessage(tr("Room logging is now disabled"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NOW_NON_ANONYMOUS))
		{
			showStatusMessage(tr("The room is now non-anonymous"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NOW_SEMI_ANONYMOUS))
		{
			showStatusMessage(tr("The room is now semi-anonymous"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NOW_FULLY_ANONYMOUS))
		{
			showStatusMessage(tr("The room is now fully-anonymous"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_ROOM_CREATED))
		{
			showStatusMessage(tr("A new room has been created"),IMessageContentOptions::TypeNotification);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_NICK_CHANGED))
		{
			shown = true;
		}
		if (ACodes.contains(MUC_SC_USER_BANNED))
		{
			shown = true;
		}
		if (ACodes.contains(MUC_SC_ROOM_ENTER))
		{
			shown = true;
		}
		if (ACodes.contains(MUC_SC_USER_KICKED))
		{
			shown = true;
		}
		if (ACodes.contains(MUC_SC_AFFIL_CHANGE))
		{
			showStatusMessage(tr("%1 has been removed from the room because of an affiliation change").arg(ANick),IMessageContentOptions::TypeEvent);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_MEMBERS_ONLY))
		{
			showStatusMessage(tr("%1 has been removed from the room because the room has been changed to members-only").arg(ANick),IMessageContentOptions::TypeEvent);
			shown = true;
		}
		if (ACodes.contains(MUC_SC_SYSTEM_SHUTDOWN))
		{
			showStatusMessage(tr("%1 is being removed from the room because of a system shutdown").arg(ANick),IMessageContentOptions::TypeEvent);
			shown = true;
		}
	}
	return shown;
}

void MultiUserChatWindow::highlightUserRole(IMultiUser *AUser)
{
	QStandardItem *userItem = FUsers.value(AUser);
	if (userItem)
	{
		QColor itemColor;
		QFont itemFont = userItem->font();
		QString role = AUser->data(MUDR_ROLE).toString();
		if (role == MUC_ROLE_MODERATOR)
		{
			itemFont.setBold(true);
			itemColor = ui.ltvUsers->palette().color(QPalette::Active, QPalette::Text);
		}
		else if (role == MUC_ROLE_PARTICIPANT)
		{
			itemFont.setBold(false);
			itemColor = ui.ltvUsers->palette().color(QPalette::Active, QPalette::Text);
		}
		else
		{
			itemFont.setBold(false);
			itemColor = ui.ltvUsers->palette().color(QPalette::Disabled, QPalette::Text);
		}
		userItem->setFont(itemFont);
		userItem->setForeground(itemColor);
	}
}

void MultiUserChatWindow::highlightUserAffiliation(IMultiUser *AUser)
{
	QStandardItem *userItem = FUsers.value(AUser);
	if (userItem)
	{
		QFont itemFont = userItem->font();
		QString affilation = AUser->data(MUDR_AFFILIATION).toString();
		if (affilation == MUC_AFFIL_OWNER)
		{
			itemFont.setStrikeOut(false);
			itemFont.setUnderline(true);
			itemFont.setItalic(false);
		}
		else if (affilation == MUC_AFFIL_ADMIN)
		{
			itemFont.setStrikeOut(false);
			itemFont.setUnderline(false);
			itemFont.setItalic(false);
		}
		else if (affilation == MUC_AFFIL_MEMBER)
		{
			itemFont.setStrikeOut(false);
			itemFont.setUnderline(false);
			itemFont.setItalic(false);
		}
		else if (affilation == MUC_AFFIL_OUTCAST)
		{
			itemFont.setStrikeOut(true);
			itemFont.setUnderline(false);
			itemFont.setItalic(false);
		}
		else
		{
			itemFont.setStrikeOut(false);
			itemFont.setUnderline(false);
			itemFont.setItalic(true);
		}
		userItem->setFont(itemFont);
	}
}

void MultiUserChatWindow::setToolTipForUser(IMultiUser *AUser)
{
	QStandardItem *userItem = FUsers.value(AUser);
	if (userItem)
	{
		QStringList toolTips;
		toolTips.append(Qt::escape(AUser->nickName()));

		Jid realJid = AUser->data(MUDR_REAL_JID).toString();
		if (!realJid.isEmpty())
			toolTips.append(Qt::escape(realJid.uFull()));

		QString role = AUser->data(MUDR_ROLE).toString();
		if (!role.isEmpty())
			toolTips.append(tr("Role: %1").arg(Qt::escape(role)));

		QString affiliation = AUser->data(MUDR_AFFILIATION).toString();
		if (!affiliation.isEmpty())
			toolTips.append(tr("Affiliation: %1").arg(Qt::escape(affiliation)));

		QString status = AUser->data(MUDR_STATUS).toString();
		if (!status.isEmpty())
			toolTips.append(QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Status:")).arg(Qt::escape(status).replace("\n","<br>")));

		userItem->setToolTip("<span>"+toolTips.join("<p/>")+"</span>");
	}
}

bool MultiUserChatWindow::execShortcutCommand(const QString &AText)
{
	bool hasCommand = false;
	if (AText.startsWith("/kick "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->userByNick(nick))
			FMultiChat->setRole(nick,MUC_ROLE_NONE,parts.join(" "));
		else
			showStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/ban "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->userByNick(nick))
			FMultiChat->setAffiliation(nick,MUC_AFFIL_OUTCAST,parts.join(" "));
		else
			showStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/invite "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		Jid userJid = Jid::fromUserInput(parts.takeFirst());
		if (userJid.isValid())
			FMultiChat->inviteContact(userJid,parts.join(" "));
		else
			showStatusMessage(tr("%1 is not valid contact JID").arg(userJid.uFull()),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/join "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		Jid joinRoomJid = Jid::fromUserInput(parts.takeFirst() + "@" + FMultiChat->roomJid().domain());
		if (joinRoomJid.isValid())
			FChatPlugin->showJoinMultiChatDialog(streamJid(),joinRoomJid,FMultiChat->nickName(),parts.join(" "));
		else
			showStatusMessage(tr("%1 is not valid room JID").arg(joinRoomJid.uBare()),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/msg "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->userByNick(nick))
		{
			Message message;
			message.setBody(parts.join(" "));
			FMultiChat->sendMessage(message,nick);
		}
		else
		{
			showStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/nick "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		FMultiChat->setNickName(nick);
		hasCommand = true;
	}
	else if (AText.startsWith("/part ") || AText.startsWith("/leave ") || AText=="/part" || AText=="/leave")
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString status = parts.join(" ");
		FMultiChat->setPresence(IPresence::Offline,status);
		exitAndDestroy(QString::null);
		hasCommand = true;
	}
	else if (AText.startsWith("/topic "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString subject = parts.join(" ");
		FMultiChat->setSubject(subject);
		hasCommand = true;
	}
	else if (AText == "/help")
	{
		showStatusMessage(tr("Supported list of commands: \n"
			" /ban <roomnick> [comment] \n"
			" /invite <jid> [comment] \n"
			" /join <roomname> [pass] \n"
			" /kick <roomnick> [comment] \n"
			" /msg <roomnick> <foo> \n"
			" /nick <newnick> \n"
			" /leave [comment] \n"
			" /topic <foo>"),IMessageContentOptions::TypeNotification);
		hasCommand = true;
	}
	return hasCommand;
}

bool MultiUserChatWindow::isMentionMessage(const Message &AMessage) const
{
	QRegExp mention(QString("\\b%1\\b").arg(QRegExp::escape(FMultiChat->nickName())));
	return AMessage.body().indexOf(mention)>=0;
}

void MultiUserChatWindow::setMessageStyle()
{
	if (FMessageStyles)
	{
		IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::GroupChat);
		if (FViewWidget->messageStyle()==NULL || !FViewWidget->messageStyle()->changeOptions(FViewWidget->styleWidget(),soptions,true))
		{
			IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
			FViewWidget->setMessageStyle(style,soptions);
		}
		FWindowStatus[FViewWidget].lastDateSeparator = QDate();
	}
}

void MultiUserChatWindow::showTopic(const QString &ATopic)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindTopic;
	options.type |= IMessageContentOptions::TypeGroupchat;
	options.direction = IMessageContentOptions::DirectionIn;

	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendText(ATopic,options);
}

void MultiUserChatWindow::showStatusMessage(const QString &AMessage, int AType, int AStatus, bool AArchive)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindStatus;
	options.type |= AType;
	options.status = AStatus;
	options.direction = IMessageContentOptions::DirectionIn;

	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (AArchive && FMessageArchiver && Options::node(OPV_MUC_GROUPCHAT_ARCHIVESTATUS).value().toBool())
		FMessageArchiver->saveNote(FMultiChat->streamJid(), FMultiChat->roomJid(), AMessage);

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendText(AMessage,options);
}

void MultiUserChatWindow::showUserMessage(const Message &AMessage, const QString &ANick)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindMessage;
	options.type |= IMessageContentOptions::TypeGroupchat;

	options.time = AMessage.dateTime();
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyles->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (AMessage.isDelayed())
		options.type |= IMessageContentOptions::TypeHistory;

	options.senderName = Qt::escape(ANick);
	options.senderId = options.senderName;

	IMultiUser *user = FMultiChat->nickName()!=ANick ? FMultiChat->userByNick(ANick) : FMultiChat->mainUser();
	if (user)
		options.senderIcon = FMessageStyles->contactIcon(user->contactJid(),user->data(MUDR_SHOW).toInt(),SUBSCRIPTION_BOTH,false);
	else
		options.senderIcon = FMessageStyles->contactIcon(Jid::null,IPresence::Offline,SUBSCRIPTION_BOTH,false);

	if (FMultiChat->nickName() != ANick)
	{
		options.direction = IMessageContentOptions::DirectionIn;
		if (isMentionMessage(AMessage))
			options.type |= IMessageContentOptions::TypeMention;
	}
	else
	{
		options.direction = IMessageContentOptions::DirectionOut;
	}

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendMessage(AMessage,options);
}

void MultiUserChatWindow::showHistory()
{
	if (FMessageArchiver && !FHistoryRequests.values().contains(NULL))
	{
		IArchiveRequest request;
		request.with = FMultiChat->roomJid();
		request.exactmatch = true;
		request.order = Qt::DescendingOrder;
		request.start = FWindowStatus.value(FViewWidget).createTime;
		request.end = QDateTime::currentDateTime();

		QString reqId = FMessageArchiver->loadMessages(FMultiChat->streamJid(),request);
		if (!reqId.isEmpty())
		{
			showStatusMessage(tr("Loading history..."),IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusEmpty,false);
			FHistoryRequests.insert(reqId,NULL);
		}
	}
}

void MultiUserChatWindow::updateWindow()
{
	QIcon icon;
	if (tabPageNotifier() && tabPageNotifier()->activeNotify()>0)
		icon = tabPageNotifier()->notifyById(tabPageNotifier()->activeNotify()).icon;
	if (icon.isNull())
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_CONFERENCE);

	QString roomName = tr("%1 (%2)").arg(FMultiChat->roomJid().uNode()).arg(FUsers.count());

	setWindowIcon(icon);
	setWindowIconText(roomName);
	setWindowTitle(tr("%1 - Conference").arg(roomName));

	ui.lblRoom->setText(QString("<big><b>%1</b></big> - %2").arg(Qt::escape(FMultiChat->roomJid().uBare())).arg(Qt::escape(FMultiChat->nickName())));

	emit tabPageChanged();
}

void MultiUserChatWindow::refreshCompleteNicks()
{
	QString curNick = FCompleteIt!=FCompleteNicks.constEnd() ? *FCompleteIt : QString::null;

	QMultiMap<QString,QString> sortedNicks;
	foreach(IMultiUser *user, FUsers.keys())
	{
		if (user != FMultiChat->mainUser())
			if (FCompleteNickStarts.isEmpty() || user->nickName().toLower().startsWith(FCompleteNickStarts))
				sortedNicks.insertMulti(user->nickName().toLower(), user->nickName());
	}
	FCompleteNicks = sortedNicks.values();

	int curNickIndex = FCompleteNicks.indexOf(curNick);
	FCompleteIt = curNickIndex>=0 ? FCompleteNicks.constBegin()+curNickIndex : FCompleteNicks.constEnd();
}

void MultiUserChatWindow::updateListItem(const Jid &AContactJid)
{
	IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
	QStandardItem *userItem = FUsers.value(user);
	if (userItem)
	{
		IChatWindow *window = findChatWindow(AContactJid);
		if (FActiveChatMessages.contains(window))
			userItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_PRIVATE_MESSAGE));
		else if (FStatusIcons)
			userItem->setIcon(FStatusIcons->iconByJidStatus(AContactJid,user->data(MUDR_SHOW).toInt(),QString::null,false));
	}
}

void MultiUserChatWindow::removeActiveMessages()
{
	if (FMessageProcessor)
		foreach(int messageId, FActiveMessages)
			FMessageProcessor->removeMessageNotify(messageId);
	FActiveMessages.clear();
}

void MultiUserChatWindow::setChatMessageStyle(IChatWindow *AWindow)
{
	if (FMessageStyles && AWindow)
	{
		IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
		if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true))
		{
			IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
			AWindow->viewWidget()->setMessageStyle(style,soptions);
		}
		FWindowStatus[AWindow->viewWidget()].lastDateSeparator = QDate();
	}
}

void MultiUserChatWindow::fillChatContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
	IMultiUser *user = AOptions.direction==IMessageContentOptions::DirectionIn ? FMultiChat->userByNick(AWindow->contactJid().resource()) : FMultiChat->mainUser();
	if (user)
		AOptions.senderIcon = FMessageStyles->contactIcon(user->contactJid(),user->data(MUDR_SHOW).toInt(),SUBSCRIPTION_BOTH,false);

	if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		AOptions.senderColor = "blue";
		AOptions.senderName = Qt::escape(AWindow->contactJid().resource());
	}
	else
	{
		AOptions.senderColor = "red";
		AOptions.senderName = Qt::escape(FMultiChat->nickName());
	}
	AOptions.senderId = AOptions.senderName;
}

void MultiUserChatWindow::showChatStatus(IChatWindow *AWindow, const QString &AMessage, int AStatus)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindStatus;
	options.status = AStatus;
	options.direction = IMessageContentOptions::DirectionIn;

	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	fillChatContentOptions(AWindow,options);
	showDateSeparator(AWindow->viewWidget(),options.time);
	AWindow->viewWidget()->appendText(AMessage,options);
}

void MultiUserChatWindow::showChatMessage(IChatWindow *AWindow, const Message &AMessage)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindMessage;

	options.time = AMessage.dateTime();
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyles->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyles->timeFormat(options.time);

	options.direction = AWindow->contactJid()!=AMessage.to() ? IMessageContentOptions::DirectionIn : IMessageContentOptions::DirectionOut;

	if (options.time.secsTo(FWindowStatus.value(AWindow->viewWidget()).createTime)>HISTORY_TIME_DELTA)
		options.type |= IMessageContentOptions::TypeHistory;

	fillChatContentOptions(AWindow,options);
	showDateSeparator(AWindow->viewWidget(),options.time);
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

void MultiUserChatWindow::showChatHistory(IChatWindow *AWindow)
{
	if (FMessageArchiver && !FHistoryRequests.values().contains(AWindow))
	{
		WindowStatus &wstatus = FWindowStatus[AWindow->viewWidget()];

		IArchiveRequest request;
		request.with = AWindow->contactJid();
		request.order = Qt::DescendingOrder;
		if (wstatus.createTime.secsTo(QDateTime::currentDateTime()) > HISTORY_TIME_DELTA)
			request.start = wstatus.startTime.isValid() ? wstatus.startTime : wstatus.createTime;
		else
			request.maxItems = HISTORY_MESSAGES;
		request.end = QDateTime::currentDateTime();

		QString reqId = FMessageArchiver->loadMessages(AWindow->streamJid(),request);
		if (!reqId.isEmpty())
		{
			showChatStatus(AWindow,tr("Loading history..."));
			FHistoryRequests.insert(reqId,AWindow);
		}
	}
}

IChatWindow *MultiUserChatWindow::getChatWindow(const Jid &AContactJid)
{
	IChatWindow *window = findChatWindow(AContactJid);
	IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
	if (!window && user && user!=FMultiChat->mainUser())
	{
		window = FMessageWidgets!=NULL ? FMessageWidgets->newChatWindow(streamJid(),AContactJid) : NULL;
		if (window)
		{
			window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

			connect(window->instance(),SIGNAL(messageReady()),SLOT(onChatMessageReady()));
			connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
			connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onChatWindowClosed()));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onChatWindowDestroyed()));
			connect(window->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onChatNotifierActiveNotifyChanged(int)));

			window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactName,false);
			window->infoWidget()->setField(IInfoWidget::ContactName,user->nickName());
			window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactShow,false);
			window->infoWidget()->setField(IInfoWidget::ContactShow,user->data(MUDR_SHOW));
			window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactStatus,false);
			window->infoWidget()->setField(IInfoWidget::ContactStatus,user->data(MUDR_STATUS));
			window->infoWidget()->autoUpdateFields();

			FChatWindows.append(window);
			FWindowStatus[window->viewWidget()].createTime = QDateTime::currentDateTime();
			updateChatWindow(window);

			Action *clearAction = new Action(window->instance());
			clearAction->setText(tr("Clear chat window"));
			clearAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
			clearAction->setShortcutId(SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW);
			connect(clearAction,SIGNAL(triggered(bool)),SLOT(onClearChatWindowActionTriggered(bool)));
			window->toolBarWidget()->toolBarChanger()->insertAction(clearAction, TBG_MWTBW_CLEAR_WINDOW);

			UserContextMenu *menu = new UserContextMenu(this,window);
			menu->menuAction()->setIcon(RSR_STORAGE_MENUICONS, MNI_MUC_USER_MENU);
			QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
			button->setPopupMode(QToolButton::InstantPopup);

			setChatMessageStyle(window);
			showChatHistory(window);
			emit chatWindowCreated(window);
		}
	}
	return window;
}

void MultiUserChatWindow::removeActiveChatMessages(IChatWindow *AWindow)
{
	if (FActiveChatMessages.contains(AWindow))
	{
		if (FMessageProcessor)
			foreach(int messageId, FActiveChatMessages.values(AWindow))
				FMessageProcessor->removeMessageNotify(messageId);
		FActiveChatMessages.remove(AWindow);
		updateListItem(AWindow->contactJid());
	}
}

void MultiUserChatWindow::updateChatWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
		icon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;
	if (FStatusIcons && icon.isNull())
		icon = FStatusIcons->iconByJidStatus(AWindow->contactJid(),AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt(),QString::null,false);

	QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
	QString caption = QString("[%1]").arg(contactName);
	AWindow->updateWindow(icon,caption,tr("%1 - Private chat").arg(caption),QString::null);
}

bool MultiUserChatWindow::event(QEvent *AEvent)
{
	if (FEditWidget && AEvent->type()==QEvent::KeyPress)
	{
		static QKeyEvent *sentEvent = NULL;
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
		if (sentEvent!=keyEvent && !keyEvent->text().isEmpty())
		{
			sentEvent = keyEvent;
			FEditWidget->textEdit()->setFocus();
			QCoreApplication::sendEvent(FEditWidget->textEdit(),AEvent);
			sentEvent = NULL;
		}
	}
	else if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QMainWindow::event(AEvent);
}

void MultiUserChatWindow::showEvent(QShowEvent *AEvent)
{
	if (isWindow())
	{
		if (!FShownDetached)
			loadWindowGeometry();
		FShownDetached = true;
		Shortcuts::insertWidgetShortcut(SCT_MESSAGEWINDOWS_CLOSEWINDOW,this);
	}
	else
	{
		FShownDetached = false;
		Shortcuts::removeWidgetShortcut(SCT_MESSAGEWINDOWS_CLOSEWINDOW,this);
	}
	QMainWindow::showEvent(AEvent);
	if (FEditWidget)
		FEditWidget->textEdit()->setFocus();
	if (isActiveTabPage())
		emit tabPageActivated();
}

void MultiUserChatWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	if (Options::node(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE).value().toBool())
		exitAndDestroy(QString::null);
	emit tabPageClosed();
}

bool MultiUserChatWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject == ui.ltvUsers->viewport())
	{
		if (AEvent->type() == QEvent::ContextMenu)
		{
			QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(AEvent);
			QStandardItem *userItem = FUsersModel->itemFromIndex(FUsersProxy->mapToSource(ui.ltvUsers->indexAt(menuEvent->pos())));
			IMultiUser *user = FUsers.key(userItem,NULL);
			if (user && user!=FMultiChat->mainUser())
			{
				Menu *menu = new Menu(this);
				menu->setAttribute(Qt::WA_DeleteOnClose,true);
				contextMenuForUser(user,menu);
				if (!menu->isEmpty())
					menu->popup(menuEvent->globalPos());
				else
					delete menu;
			}
		}
		else if (AEvent->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(AEvent);
			if (FEditWidget && mouseEvent->button()==Qt::MidButton)
			{
				QStandardItem *userItem = FUsersModel->itemFromIndex(FUsersProxy->mapToSource(ui.ltvUsers->indexAt(mouseEvent->pos())));
				if (userItem)
				{
					QString sufix = FEditWidget->textEdit()->textCursor().atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFIX).value().toString() : " ";
					FEditWidget->textEdit()->textCursor().insertText(userItem->text() + sufix);
					FEditWidget->textEdit()->setFocus();
					AEvent->accept();
					return true;
				}
			}
		}
	}
	else if (AObject == ui.sprHSplitter)
	{
		if (AEvent->type() == QEvent::Resize)
		{
			int usersIndex = ui.sprHSplitter->indexOf(ui.ltvUsers);
			QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(AEvent);
			if (resizeEvent && FUsersListWidth>0 && usersIndex>0 && resizeEvent->oldSize().width()>0)
			{
				double k = (double)resizeEvent->size().width() / resizeEvent->oldSize().width();
				QList<int> sizes = ui.sprHSplitter->sizes();
				for (int i=0; i<sizes.count(); i++)
					sizes[i] = qRound(sizes[i]*k);
				int delta = sizes.value(usersIndex) - FUsersListWidth;
				if (delta != 0)
				{
					sizes[0] += delta;
					sizes[usersIndex] -= delta;
					ui.sprHSplitter->setSizes(sizes);
				}
			}
		}
	}
	return QMainWindow::eventFilter(AObject,AEvent);
}

void MultiUserChatWindow::onChatOpened()
{
	if (FMultiChat->statusCodes().contains(MUC_SC_ROOM_CREATED))
		FMultiChat->requestConfigForm();
	setMessageStyle();
}

void MultiUserChatWindow::onChatNotify(const QString &ANotify)
{
	showStatusMessage(tr("Notify: %1").arg(ANotify),IMessageContentOptions::TypeNotification);
}

void MultiUserChatWindow::onChatError(const QString &AMessage)
{
	showStatusMessage(tr("Error: %1").arg(AMessage),IMessageContentOptions::TypeNotification,IMessageContentOptions::StatusError);
}

void MultiUserChatWindow::onChatClosed()
{
	if (!FDestroyOnChatClosed)
	{
		if (FMultiChat->show()==IPresence::Error && FMultiChat->roomError().conditionCode()==XmppStanzaError::EC_CONFLICT && !FMultiChat->nickName().endsWith("/"+FMultiChat->streamJid().resource()))
		{
			FMultiChat->setNickName(FMultiChat->nickName()+"/"+FMultiChat->streamJid().resource());
			FEnterRoom->trigger();
		}
		else
		{
			showStatusMessage(tr("Disconnected"),IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusOffline);
		}
		updateWindow();
	}
	else
	{
		deleteLater();
	}
}

void MultiUserChatWindow::onStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	Q_UNUSED(ABefore);
	FViewWidget->setStreamJid(AAfter);
	FEditWidget->setStreamJid(AAfter);
}

void MultiUserChatWindow::onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus)
{
	QString enterMessage;
	QString statusMessage;
	QStandardItem *userItem = FUsers.value(AUser);
	if (AShow!=IPresence::Offline && AShow!=IPresence::Error)
	{
		QString show = FStatusChanger ? FStatusChanger->nameByShow(AShow) : QString::null;
		if (userItem == NULL)
		{
			userItem = new QStandardItem(AUser->nickName());
			FUsersModel->appendRow(userItem);
			FUsers.insert(AUser,userItem);

			refreshCompleteNicks();
			highlightUserRole(AUser);
			highlightUserAffiliation(AUser);

			if (FMultiChat->isOpen() && Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS).value().toBool())
			{
				Jid realJid = AUser->data(MUDR_REAL_JID).toString();
				if (!realJid.isEmpty())
					enterMessage = tr("%1 <%2> has joined the room").arg(AUser->nickName()).arg(realJid.uFull());
				else
					enterMessage = tr("%1 has joined the room").arg(AUser->nickName());
				if (!AStatus.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
					enterMessage += QString(" - [%1] %2").arg(show).arg(AStatus);
				showStatusMessage(enterMessage,IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusJoined);
			}
		}
		else
		{
			UserStatus &userStatus = FUserStatus[AUser];
			if (Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool() && userStatus.lastStatusShow!=AStatus+show)
			{
				statusMessage = tr("%1 changed status to [%2] %3").arg(AUser->nickName()).arg(show).arg(AStatus);
				showStatusMessage(statusMessage);
			}
			userStatus.lastStatusShow = AStatus+show;
		}
		showStatusCodes(AUser->nickName(),FMultiChat->statusCodes());
		setToolTipForUser(AUser);
		updateListItem(AUser->contactJid());
	}
	else if (userItem)
	{
		if (!showStatusCodes(AUser->nickName(),FMultiChat->statusCodes()))
		{
			if (FMultiChat->isOpen() && Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS).value().toBool())
			{
				Jid realJid = AUser->data(MUDR_REAL_JID).toString();
				if (!realJid.isEmpty())
					enterMessage = tr("%1 <%2> has left the room").arg(AUser->nickName()).arg(realJid.uFull());
				else
					enterMessage = tr("%1 has left the room").arg(AUser->nickName());
				if (!AStatus.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
					enterMessage += " - " + AStatus.trimmed();
				showStatusMessage(enterMessage,IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusLeft);
			}
		}
		FUsers.remove(AUser);
		refreshCompleteNicks();
		qDeleteAll(FUsersModel->takeRow(userItem->row()));
	}

	if (FMultiChat->isOpen())
	{
		updateWindow();
	}

	IChatWindow *window = findChatWindow(AUser->contactJid());
	if (window)
	{
		if (FUsers.contains(AUser) || !FDestroyTimers.contains(window))
		{
			if (!enterMessage.isEmpty())
				showChatStatus(window,enterMessage,AShow!=IPresence::Offline && AShow!=IPresence::Error ? IMessageContentOptions::StatusJoined : IMessageContentOptions::StatusLeft);
			if (!statusMessage.isEmpty())
				showChatStatus(window,statusMessage);
			window->infoWidget()->setField(IInfoWidget::ContactShow,AShow);
			window->infoWidget()->setField(IInfoWidget::ContactStatus,AStatus);
			updateChatWindow(window);
		}
		else
		{
			window->instance()->deleteLater();
		}
	}
}

void MultiUserChatWindow::onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefore, const QVariant &AAfter)
{
	if (ARole == MUDR_ROLE)
	{
		if (AAfter!=MUC_ROLE_NONE && ABefore!=MUC_ROLE_NONE)
			showStatusMessage(tr("%1 role changed from %2 to %3").arg(AUser->nickName()).arg(ABefore.toString()).arg(AAfter.toString()),IMessageContentOptions::TypeEvent);
		highlightUserRole(AUser);
	}
	else if (ARole == MUDR_AFFILIATION)
	{
		if (FUsers.contains(AUser))
			showStatusMessage(tr("%1 affiliation changed from %2 to %3").arg(AUser->nickName()).arg(ABefore.toString()).arg(AAfter.toString()),IMessageContentOptions::TypeEvent);
		highlightUserAffiliation(AUser);
	}
}

void MultiUserChatWindow::onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick)
{
	QStandardItem *userItem = FUsers.value(AUser);
	if (userItem)
	{
		userItem->setText(ANewNick);
		Jid userOldJid = AUser->contactJid();
		userOldJid.setResource(AOldNick);
		IChatWindow *window = findChatWindow(userOldJid);
		if (window)
		{
			window->setContactJid(AUser->contactJid());
			window->infoWidget()->setField(IInfoWidget::ContactName,ANewNick);
			updateChatWindow(window);
		}
		refreshCompleteNicks();
	}

	if (AUser == FMultiChat->mainUser())
		updateWindow();

	showStatusMessage(tr("%1 changed nick to %2").arg(AOldNick).arg(ANewNick),IMessageContentOptions::TypeEvent);
}

void MultiUserChatWindow::onPresenceChanged(int AShow, const QString &AStatus)
{
	Q_UNUSED(AShow);
	Q_UNUSED(AStatus);
	updateStaticRoomActions();
}

void MultiUserChatWindow::onSubjectChanged(const QString &ANick, const QString &ASubject)
{
	QString topic = ANick.isEmpty() ? tr("Subject: %1").arg(ASubject) : tr("%1 has changed the subject to: %2").arg(ANick).arg(ASubject);
	showTopic(topic);
}

void MultiUserChatWindow::onServiceMessageReceived(const Message &AMessage)
{
	if (!showStatusCodes(QString::null,FMultiChat->statusCodes()))
		messageDisplay(AMessage,IMessageProcessor::MessageIn);  
}

void MultiUserChatWindow::onInviteDeclined(const Jid &AContactJid, const QString &AReason)
{
	QString nick = AContactJid && roomJid() ? AContactJid.resource() : AContactJid.uFull();
	showStatusMessage(tr("%1 has declined your invite to this room. %2").arg(nick).arg(AReason),IMessageContentOptions::TypeNotification);
}

void MultiUserChatWindow::onUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->userByNick(ANick);
	Jid realJid = user!=NULL ? user->data(MUDR_REAL_JID).toString() : Jid::null;
	showStatusMessage(tr("%1 has been kicked from the room%2. %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uFull()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageContentOptions::TypeEvent);

	if (Options::node(OPV_MUC_GROUPCHAT_REJOINAFTERKICK).value().toBool()
		&& ANick == FMultiChat->mainUser()->nickName())
	{
		QTimer::singleShot(REJOIN_AFTER_KICK_MSEC,this,SLOT(onRejoinAfterKick()));
	}
}

void MultiUserChatWindow::onRejoinAfterKick()
{
	FMultiChat->setAutoPresence(false);
	FMultiChat->setAutoPresence(true);
}

void MultiUserChatWindow::onUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->userByNick(ANick);
	Jid realJid = user!=NULL ? user->data(MUDR_REAL_JID).toString() : Jid::null;
	showStatusMessage(tr("%1 has been banned from the room%2. %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uFull()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageContentOptions::TypeEvent);
}

void MultiUserChatWindow::onAffiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList)
{
	EditUsersListDialog *dialog = new EditUsersListDialog(AAffiliation,AList,this);
	QString listName;
	if (AAffiliation == MUC_AFFIL_OUTCAST)
		listName = tr("Edit ban list - %1");
	else if (AAffiliation == MUC_AFFIL_MEMBER)
		listName = tr("Edit members list - %1");
	else if (AAffiliation == MUC_AFFIL_ADMIN)
		listName = tr("Edit administrators list - %1");
	else if (AAffiliation == MUC_AFFIL_OWNER)
		listName = tr("Edit owners list - %1");
	dialog->setTitle(listName.arg(roomJid().uBare()));
	connect(dialog,SIGNAL(accepted()),SLOT(onAffiliationListDialogAccepted()));
	connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog,SLOT(reject()));
	dialog->show();
}

void MultiUserChatWindow::onConfigFormReceived(const IDataForm &AForm)
{
	if (FDataForms)
	{
		IDataDialogWidget *dialog = FDataForms->dialogWidget(FDataForms->localizeForm(AForm),this);
		connect(dialog->instance(),SIGNAL(accepted()),SLOT(onConfigFormDialogAccepted()));
		connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog->instance(),SLOT(reject()));
		connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)),dialog->instance(),SLOT(reject()));
		dialog->instance()->show();
	}
}

void MultiUserChatWindow::onRoomDestroyed(const QString &AReason)
{
	showStatusMessage(tr("This room was destroyed by owner. %1").arg(AReason),IMessageContentOptions::TypeEvent);
}

void MultiUserChatWindow::onMessageReady()
{
	if (FMultiChat->isOpen())
	{
		Message message;

		if (FMessageProcessor)
			FMessageProcessor->textToMessage(message,FEditWidget->document());
		else
			message.setBody(FEditWidget->document()->toPlainText());

		if (!message.body().isEmpty() && FMultiChat->sendMessage(message))
			FEditWidget->clearEditor();
	}
}

void MultiUserChatWindow::onMessageAboutToBeSend()
{
	if (execShortcutCommand(FEditWidget->textEdit()->toPlainText()))
		FEditWidget->clearEditor();
}

void MultiUserChatWindow::onNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	updateWindow();
}

void MultiUserChatWindow::onEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked)
{
	if (FMultiChat->isOpen() && AKeyEvent->modifiers()+AKeyEvent->key() == NICK_MENU_KEY)
	{
		QTextEdit *textEdit = FEditWidget->textEdit();
		QTextCursor cursor = textEdit->textCursor();
		if (FCompleteIt == FCompleteNicks.constEnd())
		{
			cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
			FStartCompletePos = cursor.position();
			FCompleteNickStarts = cursor.selectedText().toLower();
			refreshCompleteNicks();
			FCompleteIt = FCompleteNicks.constBegin();
		}
		else
		{
			cursor.setPosition(FStartCompletePos, QTextCursor::KeepAnchor);
		}

		QString suffix = cursor.atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFIX).value().toString() : QString(" ");
		if (FCompleteNicks.count() > 1)
		{
			if (!Options::node(OPV_MUC_GROUPCHAT_BASHAPPEND).value().toBool())
			{
				Menu *nickMenu = new Menu(this);
				nickMenu->setAttribute(Qt::WA_DeleteOnClose,true);
				foreach(QString nick, FCompleteNicks)
				{
					IMultiUser *user = FMultiChat->userByNick(nick);
					if (user)
					{
						Action *action = new Action(nickMenu);
						action->setText(user->nickName());
						action->setIcon(FUsers.value(user)->icon());
						action->setData(ADR_USER_NICK,user->nickName());
						connect(action,SIGNAL(triggered(bool)),SLOT(onNickMenuActionTriggered(bool)));
						nickMenu->addAction(action,AG_DEFAULT,true);
					}
				}
				nickMenu->popup(textEdit->viewport()->mapToGlobal(textEdit->cursorRect().topLeft()));
			}
			else
			{
				cursor.insertText(*FCompleteIt + suffix);
				if (++FCompleteIt == FCompleteNicks.constEnd())
					FCompleteIt = FCompleteNicks.constBegin();
			}
		}
		else if (!FCompleteNicks.isEmpty())
		{
			cursor.insertText(FCompleteNicks.first() + suffix);
		}

		AHooked = true;
	}
	else
	{
		FCompleteIt = FCompleteNicks.constEnd();
	}
}

void MultiUserChatWindow::onViewContextQuoteActionTriggered(bool)
{
	QTextDocumentFragment fragment = viewWidget()->messageStyle()->selection(viewWidget()->styleWidget());
	fragment = TextManager::getTrimmedTextFragment(editWidget()->prepareTextFragment(fragment),!editWidget()->isRichTextEnabled());
	TextManager::insertQuotedFragment(editWidget()->textEdit()->textCursor(),fragment);
	editWidget()->textEdit()->setFocus();
}

void MultiUserChatWindow::onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu)
{
	Q_UNUSED(APosition);
	if (!ASelection.toPlainText().trimmed().isEmpty())
	{
		Action *quoteAction = new Action(AMenu);
		quoteAction->setText(tr("Quote selected text"));
		quoteAction->setIcon(RSR_STORAGE_MENUICONS, MNI_MESSAGEWIDGETS_QUOTE);
		quoteAction->setShortcutId(SCT_MESSAGEWINDOWS_QUOTE);
		connect(quoteAction,SIGNAL(triggered(bool)),SLOT(onViewContextQuoteActionTriggered(bool)));
		AMenu->addAction(quoteAction,AG_VWCM_MESSAGEWIDGETS_QUOTE,true);
	}
}

void MultiUserChatWindow::onWindowActivated()
{
	removeActiveMessages();
}

void MultiUserChatWindow::onChatMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window && FMultiChat->isOpen() && FMultiChat->userByNick(window->contactJid().resource())!=NULL)
	{
		Message message;
		message.setType(Message::Chat).setTo(window->contactJid().full());

		if (FMessageProcessor)
			FMessageProcessor->textToMessage(message,window->editWidget()->document());
		else
			message.setBody(window->editWidget()->document()->toPlainText());

		if (!message.body().isEmpty() && FMultiChat->sendMessage(message,window->contactJid().resource()))
			window->editWidget()->clearEditor();
	}
}

void MultiUserChatWindow::onChatWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		removeActiveChatMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
	}
}

void MultiUserChatWindow::onChatWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window && FMultiChat->userByNick(window->contactJid().resource())!=NULL)
	{
		int destroyTimeout = Options::node(OPV_MESSAGES_CLEANCHATTIMEOUT).value().toInt();
		if (destroyTimeout>0 && !FActiveChatMessages.contains(window))
		{
			if (!FDestroyTimers.contains(window))
			{
				QTimer *timer = new QTimer;
				timer->setSingleShot(true);
				connect(timer,SIGNAL(timeout()),window->instance(),SLOT(deleteLater()));
				FDestroyTimers.insert(window,timer);
			}
			FDestroyTimers[window]->start(destroyTimeout*60*1000);
		}
	}
	else if (window && !FActiveChatMessages.contains(window))
	{
		window->instance()->deleteLater();
	}
}

void MultiUserChatWindow::onChatWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (FChatWindows.contains(window))
	{
		removeActiveChatMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		FChatWindows.removeAt(FChatWindows.indexOf(window));
		FWindowStatus.remove(window->viewWidget());
		FPendingMessages.remove(window);
		FHistoryRequests.remove(FHistoryRequests.key(window));
		emit chatWindowDestroyed(window);
	}
}

void MultiUserChatWindow::onChatNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	IChatWindow *window = notifier!=NULL ? qobject_cast<IChatWindow *>(notifier->tabPage()->instance()) : NULL;
	if (window)
		updateChatWindow(window);
}

void MultiUserChatWindow::onHorizontalSplitterMoved(int APos, int AIndex)
{
	Q_UNUSED(APos);
	Q_UNUSED(AIndex);
	FUsersListWidth = ui.sprHSplitter->sizes().value(ui.sprHSplitter->indexOf(ui.ltvUsers));
}

void MultiUserChatWindow::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IChatWindow *window, FChatWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setChatMessageStyle(window);
				showChatHistory(window);
			}
		}
	}
	else if (AMessageType==Message::GroupChat && AContext.isEmpty())
	{
		IMessageStyle *style = FViewWidget!=NULL ? FViewWidget->messageStyle() : NULL;
		if (style==NULL || !style->changeOptions(FViewWidget->styleWidget(),AOptions,false))
		{
			setMessageStyle();
			showHistory();
		}
	}
}

void MultiUserChatWindow::onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);

		if (window)
			setChatMessageStyle(window);
		else
			setMessageStyle();

		int messageItEnd = 0;
		QList<Message> pendingMessages = FPendingMessages.take(window);
		while (messageItEnd<pendingMessages.count() && messageItEnd<ABody.messages.count())
		{
			const Message &hmessage = ABody.messages.at(messageItEnd);
			const Message &pmessage = pendingMessages.at(pendingMessages.count()-messageItEnd-1);
			if (hmessage.body()==pmessage.body() && qAbs(hmessage.dateTime().secsTo(pmessage.dateTime()))<HISTORY_DUBLICATE_DELTA)
				messageItEnd++;
			else
				break;
		}

		int messageIt = ABody.messages.count()-1;
		QMultiMap<QDateTime,QString>::const_iterator noteIt = ABody.notes.constBegin();
		while (messageIt>=messageItEnd || noteIt!=ABody.notes.constEnd())
		{
			if (messageIt>=messageItEnd && (noteIt==ABody.notes.constEnd() || ABody.messages.at(messageIt).dateTime()<noteIt.key()))
			{
				const Message &message = ABody.messages.at(messageIt);
				if (window)
					showChatMessage(window,message);
				else
					showUserMessage(message,Jid(message.from()).resource());
				messageIt--;
			}
			else if (noteIt != ABody.notes.constEnd())
			{
				if (window)
					showChatStatus(window,noteIt.value());
				else
					showStatusMessage(noteIt.value(),IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusEmpty,false);
				noteIt++;
			}
		}

		if (!window)
			showTopic(FMultiChat->subject());

		foreach(Message message, pendingMessages)
		{
			if (window)
				showChatMessage(window,message);
			else
				showUserMessage(message,Jid(message.from()).resource());
		}

		if (window)
		{
			WindowStatus &wstatus = FWindowStatus[window->viewWidget()];
			wstatus.startTime = !ABody.messages.isEmpty() ? ABody.messages.last().dateTime() : QDateTime();
		}
	}
}

void MultiUserChatWindow::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (window)
			showChatStatus(window,tr("Failed to load history: %1").arg(AError));
		else
			showStatusMessage(tr("Failed to load history: %1").arg(AError),IMessageContentOptions::TypeEmpty,IMessageContentOptions::StatusEmpty,false);
		FPendingMessages.remove(window);
	}
}

void MultiUserChatWindow::onNickMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString nick = action->data(ADR_USER_NICK).toString();
		QTextCursor cursor = FEditWidget->textEdit()->textCursor();
		cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
		QString sufix = cursor.atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFIX).value().toString() : " ";
		cursor.insertText(nick + sufix);
	}
}

void MultiUserChatWindow::onToolBarActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FChangeNick)
	{
		QString nick = QInputDialog::getText(this,tr("Change nick name"),tr("Enter your new nick name in room %1").arg(roomJid().uNode()),
			QLineEdit::Normal,FMultiChat->nickName());
		if (!nick.isEmpty())
			FMultiChat->setNickName(nick);
	}
	else if (action == FChangeSubject)
	{
		if (FMultiChat->isOpen())
		{
			QString newSubject = FMultiChat->subject();
			InputTextDialog *dialog = new InputTextDialog(this,tr("Change subject"),tr("Enter new subject for room %1").arg(roomJid().uNode()), newSubject);
			if (dialog->exec() == QDialog::Accepted)
				FMultiChat->setSubject(newSubject);
		}
	}
	else if (action == FClearChat)
	{
		setMessageStyle();
	}
	else if (action == FEnterRoom)
	{
		FMultiChat->setAutoPresence(false);
		FMultiChat->setAutoPresence(true);
	}
	else if (action == FExitRoom)
	{
		exitAndDestroy(QString::null);
	}
	else if (action == FInviteContact)
	{
		if (FMultiChat->isOpen())
		{
			Jid contactJid = QInputDialog::getText(this,tr("Invite user"),tr("Enter user JID:"));
			if (contactJid.isValid())
			{
				QString reason = tr("You are welcome here");
				reason = QInputDialog::getText(this,tr("Invite user"),tr("Enter a reason:"),QLineEdit::Normal,reason);
				FMultiChat->inviteContact(contactJid,reason);
			}
		}
	}
	else if (action == FRequestVoice)
	{
		FMultiChat->requestVoice();
	}
	else if (action == FBanList)
	{
		FMultiChat->requestAffiliationList(MUC_AFFIL_OUTCAST);
	}
	else if (action == FMembersList)
	{
		FMultiChat->requestAffiliationList(MUC_AFFIL_MEMBER);
	}
	else if (action == FAdminsList)
	{
		FMultiChat->requestAffiliationList(MUC_AFFIL_ADMIN);
	}
	else if (action == FOwnersList)
	{
		FMultiChat->requestAffiliationList(MUC_AFFIL_OWNER);
	}
	else if (action == FConfigRoom)
	{
		FMultiChat->requestConfigForm();
	}
	else if (action == FDestroyRoom)
	{
		if (FMultiChat->isOpen())
		{
			bool ok = false;
			QString reason = QInputDialog::getText(this,tr("Destroying room"),tr("Enter a reason:"),QLineEdit::Normal,QString::null,&ok);
			if (ok)
				FMultiChat->destroyRoom(reason);
		}
	}
}

void MultiUserChatWindow::onOpenChatWindowActionTriggered(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUser *user = FMultiChat->userByNick(action->data(ADR_USER_NICK).toString());
		if (user)
			openChatWindow(user->contactJid());
	}
}

void MultiUserChatWindow::onChangeUserRoleActionTriggeted(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		bool ok = true;
		QString reason;
		QString role = action->data(ADR_USER_ROLE).toString();
		if (role == MUC_ROLE_NONE)
			reason = QInputDialog::getText(this,tr("Kick reason"),tr("Enter reason for kick"),QLineEdit::Normal,QString::null,&ok);
		if (ok)
			FMultiChat->setRole(action->data(ADR_USER_NICK).toString(),role,reason);
	}
}

void MultiUserChatWindow::onChangeUserAffiliationActionTriggered(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		bool ok = true;
		QString reason;
		QString affil = action->data(ADR_USER_AFFIL).toString();
		if (affil == MUC_AFFIL_OUTCAST)
			reason = QInputDialog::getText(this,tr("Ban reason"),tr("Enter reason for ban"),QLineEdit::Normal,QString::null,&ok);
		if (ok)
			FMultiChat->setAffiliation(action->data(ADR_USER_NICK).toString(),affil,reason);
	}
}

void MultiUserChatWindow::onClearChatWindowActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IChatWindow *window = action!=NULL ? qobject_cast<IChatWindow *>(action->parent()) : NULL;
	if (window)
	{
		IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
		if (style!=NULL)
		{
			IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
			style->changeOptions(window->viewWidget()->styleWidget(),soptions,true);
		}
	}
}

void MultiUserChatWindow::onDataFormMessageDialogAccepted()
{
	IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
	if (dialog)
		FMultiChat->sendDataFormMessage(FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
}

void MultiUserChatWindow::onAffiliationListDialogAccepted()
{
	EditUsersListDialog *dialog = qobject_cast<EditUsersListDialog *>(sender());
	if (dialog)
		FMultiChat->changeAffiliationList(dialog->deltaList());
}

void MultiUserChatWindow::onConfigFormDialogAccepted()
{
	IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
	if (dialog)
		FMultiChat->sendConfigForm(FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
}

void MultiUserChatWindow::onUserItemDoubleClicked(const QModelIndex &AIndex)
{
	IMultiUser *user = FUsers.key(FUsersModel->itemFromIndex(FUsersProxy->mapToSource(AIndex)));
	if (user)
		openChatWindow(user->contactJid());
}

void MultiUserChatWindow::onStatusIconsChanged()
{
	foreach(IChatWindow *window, FChatWindows) {
		updateChatWindow(window);	}
	foreach(IMultiUser *user, FUsers.keys()) {
		updateListItem(user->contactJid());	}
	updateWindow();
}

void MultiUserChatWindow::onAccountOptionsChanged(const OptionsNode &ANode)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account && account->optionsNode().childPath(ANode) == "name")
		ui.lblAccount->setText(ANode.value().toString());
}

void MultiUserChatWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
	{
		closeTabPage();
	}
}
