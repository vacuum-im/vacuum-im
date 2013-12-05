#include "normalmessagehandler.h"

#include <QLineEdit>
#include <QMouseEvent>

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_GROUP                 Action::DR_Parametr2
#define ADR_WINDOW                Action::DR_Parametr1
#define ADR_ACTION_ID             Action::DR_Parametr2

NormalMessageHandler::NormalMessageHandler()
{
	FAvatars = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FStatusIcons = NULL;
	FNotifications = NULL;
	FPresencePlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FXmppUriQueries = NULL;
	FOptionsManager = NULL;
	FRecentContacts = NULL;
}

NormalMessageHandler::~NormalMessageHandler()
{

}

void NormalMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Normal Messages");
	APluginInfo->description = tr("Allows to exchange normal messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}

bool NormalMessageHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
		{
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
		if (FAvatars)
		{
			connect(FAvatars->instance(),SIGNAL(avatarChanged(const Jid &)),SLOT(onAvatarChanged(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
		FNotifications = qobject_cast<INotifications *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRecentContacts").value(0,NULL);
	if (plugin)
		FRecentContacts = qobject_cast<IRecentContacts *>(plugin->instance());

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool NormalMessageHandler::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWNORMALDIALOG, tr("Send message"), tr("Ctrl+Return","Send message"), Shortcuts::WidgetShortcut);

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_NORMALHANDLER_MESSAGE;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMALMHANDLER_MESSAGE);
		notifyType.title = tr("When receiving new single message");
		notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_NORMAL_MESSAGE,notifyType);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_NORMALMESSAGEHANDLER,this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this,XUHO_DEFAULT);
	}
	if (FRostersView)
	{
		FRostersView->insertClickHooker(RCHO_NORMALMESSAGEHANDLER,this);
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWNORMALDIALOG, FRostersView->instance());
	}
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	if (FMessageWidgets)
	{
		FMessageWidgets->insertEditSendHandler(MESHO_NORMALMESSAGEHANDLER,this);
	}
	return true;
}

bool NormalMessageHandler::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_UNNOTIFYALLNORMAL, false);
	return true;
}

bool NormalMessageHandler::messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget)
{
	Q_UNUSED(AOrder); Q_UNUSED(AWidget);
	return false;
}

bool NormalMessageHandler::messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget)
{
	if (AOrder == MESHO_NORMALMESSAGEHANDLER)
	{
		IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(AWidget->messageWindow()->instance());
		if (FMessageProcessor && window && window->mode()==IMessageNormalWindow::WriteMode)
		{
			Message message;
			message.setType(Message::Normal).setSubject(window->subject()).setThreadId(window->threadId());
			FMessageProcessor->textToMessage(message,AWidget->document());
			if (!message.body().isEmpty())
			{
				bool sent = false;
				QMultiMap<Jid, Jid> addresses = window->receiversWidget()->selectedAddresses();
				for (QMultiMap<Jid, Jid>::const_iterator it=addresses.constBegin(); it!=addresses.constEnd(); ++it)
				{
					message.setTo(it->full());
					sent = FMessageProcessor->sendMessage(it.key(),message,IMessageProcessor::MessageOut) ? true : sent;
				}
				return sent;
			}
		}
	}
	return false;
}

bool NormalMessageHandler::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder); Q_UNUSED(ADirection);
	return AMessage.type()!=Message::GroupChat && (!AMessage.body().isEmpty() || !AMessage.subject().isEmpty());
}

bool NormalMessageHandler::messageDisplay(const Message &AMessage, int ADirection)
{
	if (ADirection == IMessageProcessor::MessageIn)
	{
		IMessageNormalWindow *window = getWindow(AMessage.to(),AMessage.from(),IMessageNormalWindow::ReadMode);
		if (window)
		{
			if (FRecentContacts)
			{
				IRecentItem recentItem;
				recentItem.type = REIT_CONTACT;
				recentItem.streamJid = window->streamJid();
				recentItem.reference = window->contactJid().pBare();
				FRecentContacts->setItemActiveTime(recentItem);
			}

			QQueue<Message> &messages = FMessageQueue[window];
			if (messages.isEmpty())
				showStyledMessage(window,AMessage);
			messages.append(AMessage);

			updateWindow(window);
			return true;
		}
	}
	return false;
}

INotification NormalMessageHandler::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::MessageIn)
	{
		IMessageNormalWindow *window = findWindow(AMessage.to(),AMessage.from());
		if (window)
		{
			notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_NORMAL_MESSAGE);
			if (notify.kinds > 0)
			{
				QIcon icon =  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMALMHANDLER_MESSAGE);
				QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

				notify.typeId = NNT_NORMAL_MESSAGE;
				notify.data.insert(NDR_ICON,icon);
				notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
				notify.data.insert(NDR_STREAM_JID,AMessage.to());
				notify.data.insert(NDR_CONTACT_JID,AMessage.from());
				notify.data.insert(NDR_ROSTER_ORDER,RNO_NORMALMESSAGE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
				notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.from()));
				notify.data.insert(NDR_POPUP_CAPTION, tr("Message received"));
				notify.data.insert(NDR_POPUP_TITLE,name);
				notify.data.insert(NDR_SOUND_FILE,SDF_NORMAL_MHANDLER_MESSAGE);

				notify.data.insert(NDR_ALERT_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_NEW_MESSAGE);
				notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)window->instance());

				if (FMessageProcessor)
				{
					QTextDocument doc;
					FMessageProcessor->messageToText(&doc,AMessage);
					notify.data.insert(NDR_POPUP_HTML,TextManager::getDocumentBody(doc));
				}
				else
				{
					notify.data.insert(NDR_POPUP_HTML,AMessage.body().toHtmlEscaped());
				}

				FNotifiedMessages.insertMulti(window,AMessage.data(MDR_MESSAGE_ID).toInt());
			}
		}
	}
	return notify;
}

bool NormalMessageHandler::messageShowWindow(int AMessageId)
{
	IMessageNormalWindow *window = FNotifiedMessages.key(AMessageId);
	if (window == NULL)
	{
		Message message = FMessageProcessor->notifiedMessage(AMessageId);
		if (messageDisplay(message,IMessageProcessor::MessageIn))
		{
			IMessageNormalWindow *window = findWindow(message.to(),message.from());
			if (window)
			{
				FNotifiedMessages.insertMulti(window,AMessageId);
				window->showTabPage();
				return true;
			}
		}
	}
	else 
	{
		window->showTabPage();
		return true;
	}
	return false;
}

bool NormalMessageHandler::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType != Message::GroupChat)
	{
		IMessageNormalWindow *window = getWindow(AStreamJid,AContactJid,IMessageNormalWindow::WriteMode);
		if (window)
		{
			if (AShowMode == IMessageHandler::SM_ASSIGN)
				window->assignTabPage();
			else if (AShowMode == IMessageHandler::SM_SHOW)
				window->showTabPage();
			else if (AShowMode == IMessageHandler::SM_MINIMIZED)
				window->showMinimizedTabPage();
			return true;
		}
	}
	return false;
}

QMultiMap<int, IOptionsWidget *> NormalMessageHandler::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES_UNNOTIFYALLNORMAL,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_UNNOTIFYALLNORMAL),tr("Mark all single messages from user as read when you read the first one"),AParent));
	}
	return widgets;
}

bool NormalMessageHandler::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(AEvent);
	return false;
}

bool NormalMessageHandler::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	int indexKind = AIndex->kind();
	if (AOrder==RCHO_NORMALMESSAGEHANDLER && AEvent->modifiers()==Qt::NoModifier)
	{
		QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
		if (isAnyPresenceOpened(QStringList() << streamJid))
		{
			if (indexKind==RIK_STREAM_ROOT && FRostersModel && FRostersModel->streamsLayout()==IRostersModel::LayoutMerged)
			{
				return messageShowWindow(MHO_NORMALMESSAGEHANDLER,streamJid,Jid::null,Message::Normal,IMessageHandler::SM_SHOW);
			}
			else if (indexKind==RIK_CONTACT || indexKind==RIK_MY_RESOURCE || indexKind==RIK_AGENT)
			{
				Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
				return messageShowWindow(MHO_NORMALMESSAGEHANDLER,streamJid,Jid::null,Message::Normal,IMessageHandler::SM_SHOW);
			}
		}
	}
	return false;
}

bool NormalMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type.isEmpty() || type=="normal")
		{
			IMessageNormalWindow *window = getWindow(AStreamJid, AContactJid, IMessageNormalWindow::WriteMode);
			if (window)
			{
				if (AParams.contains("thread"))
					window->setThreadId(AParams.value("thread"));
				window->setSubject(AParams.value("subject"));
				window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
				window->showTabPage();
				return true;
			}
		}
	}
	return false;
}

IMessageNormalWindow *NormalMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageNormalWindow::Mode AMode)
{
	IMessageNormalWindow *window = NULL;
	if (FMessageProcessor && FMessageProcessor->isActiveStream(AStreamJid) && (AContactJid.isValid() || AMode==IMessageNormalWindow::WriteMode))
	{
		window = FMessageWidgets->getNormalWindow(AStreamJid,AContactJid,AMode);
		if (window)
		{
			window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

			connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));
			connect(window->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onWindowAddressChanged()));
			connect(window->address()->instance(),SIGNAL(availAddressesChanged()),SLOT(onWindowAvailAddressesChanged()));
			connect(window->infoWidget()->instance(),SIGNAL(contextMenuRequested(Menu *)),SLOT(onWindowContextMenuRequested(Menu *)));
			connect(window->infoWidget()->instance(),SIGNAL(toolTipsRequested(QMap<int,QString> &)),SLOT(onWindowToolTipsRequested(QMap<int,QString> &)));
			connect(window->receiversWidget()->instance(),SIGNAL(addressSelectionChanged()),SLOT(onWindowSelectedReceiversChanged()));
			connect(window->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onWindowNotifierActiveNotifyChanged(int)));
			onWindowSelectedReceiversChanged();
			
			Menu *windowMenu = createWindowMenu(window);
			QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(windowMenu->menuAction(),TBG_MWNWTB_WINDOWMENU);
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

			FWindows.append(window);
			updateWindow(window);
			setMessageStyle(window);
		}
		else
		{
			window = findWindow(AStreamJid,AContactJid);
		}
	}
	return window;
}

IMessageNormalWindow *NormalMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IMessageNormalWindow *window, FWindows)
		if (window->streamJid()==AStreamJid && window->contactJid()==AContactJid)
			return window;
	return NULL;
}

Menu *NormalMessageHandler::createWindowMenu(IMessageNormalWindow *AWindow) const
{
	Menu *menu = new Menu(AWindow->instance());

	Action *nextAction = new Action(menu);
	nextAction->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_NEXT);
	nextAction->setData(ADR_ACTION_ID,NextAction);
	nextAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(nextAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuShowNextMessage()));
	menu->addAction(nextAction);

	Action *sendAction = new Action(menu);
	sendAction->setText(tr("Send"));
	sendAction->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_SEND);
	sendAction->setData(ADR_ACTION_ID,SendAction);
	sendAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(sendAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuSendMessage()));
	menu->addAction(sendAction);

	Action *replyAction = new Action(menu);
	replyAction->setText(tr("Reply"));
	replyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_REPLY);
	replyAction->setData(ADR_ACTION_ID,ReplyAction);
	replyAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(replyAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuReplyMessage()));
	menu->addAction(replyAction);

	Action *forwardAction = new Action(menu);
	forwardAction->setText(tr("Forward"));
	forwardAction->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_FORWARD);
	forwardAction->setData(ADR_ACTION_ID,ForwardAction);
	forwardAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(forwardAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuForwardMessage()));
	menu->addAction(forwardAction);

	Action *openChatAction = new Action(menu);
	openChatAction->setText(tr("Show Chat Dialog"));
	openChatAction->setData(ADR_ACTION_ID,OpenChatAction);
	openChatAction->setIcon(RSR_STORAGE_MENUICONS,MNI_CHATMHANDLER_MESSAGE);
	openChatAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(openChatAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuShowChatDialog()));
	menu->addAction(openChatAction);

	Action *sendChatAction = new Action(menu);
	sendChatAction->setCheckable(true);
	sendChatAction->setText(tr("Send as Chat Message"));
	sendChatAction->setData(ADR_ACTION_ID,SendChatAction);
	sendChatAction->setData(ADR_WINDOW,(qint64)AWindow->instance());
	connect(sendChatAction,SIGNAL(triggered(bool)),SLOT(onWindowMenuSendAsChatMessage()));
	menu->addAction(sendChatAction,AG_DEFAULT+100);

	return menu;
}

Action *NormalMessageHandler::findWindowMenuAction(IMessageNormalWindow *AWindow, WindowMenuAction AActionId) const
{
	if (AWindow)
	{
		QAction *menuHandle = AWindow->toolBarWidget()->toolBarChanger()->groupItems(TBG_MWNWTB_WINDOWMENU).value(0);
		Action *menuAction = AWindow->toolBarWidget()->toolBarChanger()->handleAction(menuHandle);
		if (menuAction && menuAction->menu())
		{
			foreach(Action *action, menuAction->menu()->groupActions())
				if (action->data(ADR_ACTION_ID).toInt() == AActionId)
					return action;
		}
	}
	return NULL;
}

void NormalMessageHandler::setDefaultWindowMenuAction(IMessageNormalWindow *AWindow, WindowMenuAction AActionId) const
{
	Action *action = findWindowMenuAction(AWindow,AActionId);
	if (action)
	{
		Menu *menu = qobject_cast<Menu *>(action->parent());
		if (menu)
		{
			menu->menuAction()->disconnect(menu->defaultAction());
			
			menu->setDefaultAction(action);
			menu->menuAction()->setText(action->text());
			menu->menuAction()->setIcon(action->icon());
			menu->menuAction()->setEnabled(action->isEnabled());
			connect(menu->menuAction(),SIGNAL(triggered()),action,SLOT(trigger()));
		}
	}
}

void NormalMessageHandler::setWindowMenuActionVisible(IMessageNormalWindow *AWindow, WindowMenuAction AActionId, bool AVisible) const
{
	Action *action = findWindowMenuAction(AWindow,AActionId);
	if (action)
		action->setVisible(AVisible);
}

void NormalMessageHandler::setWindowMenuActionEnabled(IMessageNormalWindow *AWindow, WindowMenuAction AActionId, bool AEnabled) const
{
	Action *action = findWindowMenuAction(AWindow,AActionId);
	if (action)
		action->setEnabled(AEnabled);
}

void NormalMessageHandler::updateWindowMenu(IMessageNormalWindow *AWindow) const
{
	int nextCount = FMessageQueue.value(AWindow).count()-1;
	if (AWindow->mode() == IMessageNormalWindow::WriteMode)
	{
		Action *sendAction = findWindowMenuAction(AWindow,SendAction);
		if (sendAction)
			sendAction->setEnabled(!AWindow->receiversWidget()->selectedAddresses().isEmpty()); 

		setWindowMenuActionVisible(AWindow,NextAction,nextCount>0);
		setWindowMenuActionVisible(AWindow,SendAction,true);
		setWindowMenuActionVisible(AWindow,ReplyAction,false);
		setWindowMenuActionVisible(AWindow,ForwardAction,false);
		setWindowMenuActionVisible(AWindow,OpenChatAction,AWindow->contactJid().isValid());
		setWindowMenuActionVisible(AWindow,SendChatAction,true);
		setDefaultWindowMenuAction(AWindow,SendAction);
	}
	else
	{
		setWindowMenuActionVisible(AWindow,NextAction,nextCount>0);
		setWindowMenuActionVisible(AWindow,SendAction,false);
		setWindowMenuActionVisible(AWindow,ReplyAction,true);
		setWindowMenuActionVisible(AWindow,ForwardAction,true);
		setWindowMenuActionVisible(AWindow,OpenChatAction,AWindow->contactJid().isValid());
		setWindowMenuActionVisible(AWindow,SendChatAction,false);
		setDefaultWindowMenuAction(AWindow,nextCount>0 ? NextAction : ReplyAction);
	}
}

void NormalMessageHandler::updateWindow(IMessageNormalWindow *AWindow) const
{
	if (FAvatars)
	{
		QString avatar = FAvatars->avatarHash(AWindow->contactJid());
		if (!avatar.isEmpty() && FAvatars->hasAvatar(avatar))
			AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Avatar,avatar);
		else
			AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Avatar,FAvatars->emptyAvatarImage());
	}

	QString name = FMessageStyles!=NULL ? FMessageStyles->contactName(AWindow->streamJid(),AWindow->contactJid()) : AWindow->contactJid().uFull();
	AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Name,name);

	QIcon statusIcon;
	if (FStatusIcons && AWindow->mode()==IMessageNormalWindow::ReadMode)
		statusIcon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());
	else
		statusIcon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMALMHANDLER_MESSAGE);
	AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);

	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AWindow->streamJid()) : NULL;
	IPresenceItem pitem = presence!=NULL ? presence->findItem(AWindow->contactJid()) : IPresenceItem();
	AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusText,pitem.status);

	QString title;
	if (AWindow->mode() == IMessageNormalWindow::ReadMode)
		title = tr("%1 - Message").arg(name);
	else
		title = tr("Composing message");

	QIcon tabIcon = statusIcon;
	if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
		tabIcon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;

	int nextCount = FMessageQueue.value(AWindow).count()-1;
	if (nextCount > 0)
	{
		Action *nextAction = findWindowMenuAction(AWindow,NextAction);
		if (nextAction)
			nextAction->setText(tr("Show Next (%1)").arg(nextCount));
	}

	updateWindowMenu(AWindow);
	AWindow->updateWindow(tabIcon,name,title,QString::null);
}

bool NormalMessageHandler::showNextMessage(IMessageNormalWindow *AWindow)
{
	if (FMessageQueue.value(AWindow).count() > 1)
	{
		QQueue<Message> &messages = FMessageQueue[AWindow];
		messages.removeFirst();

		Message message = messages.head();
		showStyledMessage(AWindow,message);
		removeCurrentMessageNotify(AWindow);
		updateWindow(AWindow);
		return true;
	}
	return false;
}

void NormalMessageHandler::removeCurrentMessageNotify(IMessageNormalWindow *AWindow)
{
	if (!FMessageQueue.value(AWindow).isEmpty())
	{
		int messageId = FMessageQueue.value(AWindow).head().data(MDR_MESSAGE_ID).toInt();
		removeNotifiedMessages(AWindow,messageId);
	}
}

void NormalMessageHandler::removeNotifiedMessages(IMessageNormalWindow *AWindow, int AMessageId)
{
	foreach(int messageId, FNotifiedMessages.values(AWindow))
	{
		if (AMessageId<0 || AMessageId==messageId)
		{
			FMessageProcessor->removeMessageNotify(messageId);
			FNotifiedMessages.remove(AWindow,messageId);
		}
	}
}

void NormalMessageHandler::setMessageStyle(IMessageNormalWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Normal);
	if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,false))
	{
		IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
		AWindow->viewWidget()->setMessageStyle(style,soptions);
	}
}

void NormalMessageHandler::fillContentOptions(IMessageNormalWindow *AWindow, IMessageContentOptions &AOptions) const
{
	AOptions.senderColor = "blue";
	AOptions.senderId = AWindow->contactJid().full();
	AOptions.senderName = FMessageStyles->contactName(AWindow->streamJid(),AWindow->contactJid()).toHtmlEscaped();
	AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->contactJid());
	AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid(),AWindow->contactJid());
}

void NormalMessageHandler::showStyledMessage(IMessageNormalWindow *AWindow, const Message &AMessage)
{
	IMessageContentOptions options;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	options.noScroll = true;
	fillContentOptions(AWindow,options);

	AWindow->setMode(IMessageNormalWindow::ReadMode);
	AWindow->setSubject(AMessage.subject());
	AWindow->setThreadId(AMessage.threadId());

	AWindow->viewWidget()->clearContent();

	if (AMessage.type() == Message::Error)
	{
		XmppStanzaError err(AMessage.stanza());
		QString html = tr("<b>The message with a error is received</b>");
		html += "<p style='color:red;'>"+err.errorMessage().toHtmlEscaped()+"</p>";
		html += "<hr>";
		options.kind = IMessageContentOptions::KindMessage;
		AWindow->viewWidget()->appendHtml(html,options);
	}

	options.kind = IMessageContentOptions::KindTopic;
	AWindow->viewWidget()->appendText(tr("Subject: %1").arg(!AMessage.subject().isEmpty() ? AMessage.subject() : tr("<no subject>")),options);
	options.kind = IMessageContentOptions::KindMessage;
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

bool NormalMessageHandler::isAnyPresenceOpened(const QStringList &AStreams) const
{
	foreach(const Jid &streamJid, AStreams)
	{
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
		if (presence && presence->isOpen())
			return true;
	}
	return false;
}

bool NormalMessageHandler::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> acceptKinds = QList<int>() 
		<< RIK_STREAM_ROOT 
		<< RIK_CONTACT << RIK_AGENT << RIK_MY_RESOURCE
		<< RIK_GROUP << RIK_GROUP_BLANK	<< RIK_GROUP_NOT_IN_ROSTER;
	static const QList<int> contactKinds =  QList<int>() << RIK_CONTACT << RIK_AGENT << RIK_MY_RESOURCE;
	static const QList<int> groupKinds = QList<int>() << RIK_GROUP << RIK_GROUP_BLANK << RIK_GROUP_NOT_IN_ROSTER;

	bool hasGroups = false;
	bool hasContacts = false;
	bool hasOpenedStreams = false;
	for(int i=0; i<ASelected.count(); i++)
	{
		IRosterIndex *index = ASelected.at(i);
		int indexKind = index->kind();
		if (!acceptKinds.contains(indexKind))
			return false;
		else if (hasGroups && !groupKinds.contains(indexKind))
			return false;
		else if (hasContacts && !contactKinds.contains(indexKind))
			return false;
		else if (groupKinds.contains(indexKind) && !isAnyPresenceOpened(index->data(RDR_STREAMS).toStringList()))
			return false;
		else if (contactKinds.contains(indexKind) && !isAnyPresenceOpened(index->data(RDR_STREAM_JID).toStringList()))
			return false;
		else if (indexKind==RIK_STREAM_ROOT && isAnyPresenceOpened(index->data(RDR_STREAM_JID).toStringList()))
			hasOpenedStreams = true;
		else if (indexKind==RIK_STREAM_ROOT && !hasOpenedStreams && i==ASelected.count()-1)
			return false;
		hasGroups = hasGroups || groupKinds.contains(indexKind);
		hasContacts = hasContacts || contactKinds.contains(indexKind);
	}
	return !ASelected.isEmpty();
}

QMap<int,QStringList> NormalMessageHandler::indexesRolesMap(const QList<IRosterIndex *> &AIndexes) const
{
	QMap<int, QStringList> rolesMap;
	foreach(IRosterIndex *index, AIndexes)
	{
		int indexKind = index->kind();
		if (indexKind==RIK_GROUP || indexKind==RIK_GROUP_BLANK || indexKind==RIK_GROUP_AGENTS)
		{
			QString group;
			if (indexKind != RIK_GROUP)
				group = FRostersModel!=NULL ? FRostersModel->singleGroupName(indexKind) : QString::null;
			else
				group = index->data(RDR_GROUP).toString();

			foreach(const QString &streamJid, index->data(RDR_STREAMS).toStringList())
			{
				rolesMap[RDR_STREAM_JID].append(streamJid);
				rolesMap[RDR_PREP_BARE_JID].append(QString::null);
				rolesMap[RDR_GROUP].append(group);
			}
		}
		else if (indexKind == RIK_GROUP_NOT_IN_ROSTER)
		{
			for (int row=0; row<index->childCount(); row++)
			{
				IRosterIndex *child = index->childIndex(row);
				if (child->kind() == RIK_CONTACT)
				{
					rolesMap[RDR_STREAM_JID].append(child->data(RDR_STREAM_JID).toString());
					rolesMap[RDR_PREP_BARE_JID].append(child->data(RDR_PREP_BARE_JID).toString());
					rolesMap[RDR_GROUP].append(QString::null);
				}
			}
		}
		else if (indexKind == RIK_STREAM_ROOT)
		{
			if (isAnyPresenceOpened(index->data(RDR_STREAM_JID).toStringList()))
			{
				rolesMap[RDR_STREAM_JID].append(index->data(RDR_STREAM_JID).toString());
				rolesMap[RDR_PREP_BARE_JID].append(QString::null);
				rolesMap[RDR_GROUP].append(QString::null);
			}
		}
		else
		{
			rolesMap[RDR_STREAM_JID].append(index->data(RDR_STREAM_JID).toString());
			rolesMap[RDR_PREP_BARE_JID].append(index->data(RDR_PREP_BARE_JID).toString());
			rolesMap[RDR_GROUP].append(QString::null);
		}
	}
	return rolesMap;
}

void NormalMessageHandler::onWindowActivated()
{
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(sender());
	if (FWindows.contains(window))
	{
		if (Options::node(OPV_MESSAGES_UNNOTIFYALLNORMAL).value().toBool())
			removeNotifiedMessages(window);
		else
			removeCurrentMessageNotify(window);
	}
}

void NormalMessageHandler::onWindowDestroyed()
{
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(sender());
	if (FWindows.contains(window))
	{
		FWindows.removeAll(window);
		FMessageQueue.remove(window);
		FNotifiedMessages.remove(window);
	}
}

void NormalMessageHandler::onWindowAddressChanged()
{
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(sender()->parent());
	if (window)
		updateWindow(window);
}

void NormalMessageHandler::onWindowAvailAddressesChanged()
{
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(sender()->parent());
	if (window)
	{
		QMultiMap<Jid,Jid> addresses = window->address()->availAddresses();
		if (addresses.isEmpty())
			window->instance()->deleteLater();
	}
}

void NormalMessageHandler::onWindowSelectedReceiversChanged()
{
	IMessageReceiversWidget *widget = qobject_cast<IMessageReceiversWidget *>(sender());
	if (widget)
	{
		IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(widget->messageWindow()->instance());
		updateWindowMenu(window);
	}
}

void NormalMessageHandler::onWindowContextMenuRequested(Menu *AMenu)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	if (widget && FRostersModel && FRostersView)
	{
		IRosterIndex *index = FRostersModel->findContactIndexes(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()).value(0);
		if (index)
			FRostersView->contextMenuForIndex(QList<IRosterIndex *>()<<index,NULL,AMenu);
	}
}

void NormalMessageHandler::onWindowToolTipsRequested(QMap<int,QString> &AToolTips)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	if (widget && FRostersModel && FRostersView)
	{
		IRosterIndex *index = FRostersModel->findContactIndexes(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()).value(0);
		if (index)
			FRostersView->toolTipsForIndex(index,NULL,AToolTips);
	}
}

void NormalMessageHandler::onWindowNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	IMessageTabPageNotifier *notifier = qobject_cast<IMessageTabPageNotifier *>(sender());
	IMessageNormalWindow *window = notifier!=NULL ? qobject_cast<IMessageNormalWindow *>(notifier->tabPage()->instance()) : NULL;
	if (window)
		updateWindow(window);
}

void NormalMessageHandler::onWindowMenuSendMessage()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (window)
	{
		if (window->editWidget()->sendMessage() && !showNextMessage(window))
			window->closeTabPage();
	}
}

void NormalMessageHandler::onWindowMenuShowNextMessage()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (window)
		showNextMessage(window);
}

void NormalMessageHandler::onWindowMenuReplyMessage()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (window)
	{
		window->setMode(IMessageNormalWindow::WriteMode);
		window->setSubject(tr("Re: %1").arg(window->subject()));
		window->editWidget()->textEdit()->clear();
		window->editWidget()->textEdit()->setFocus();
		updateWindow(window);
	}
}

void NormalMessageHandler::onWindowMenuForwardMessage()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (FMessageProcessor && !FMessageQueue.value(window).isEmpty())
	{
		Message message = FMessageQueue.value(window).head();
		window->setMode(IMessageNormalWindow::WriteMode);
		window->setSubject(tr("Fw: %1").arg(message.subject()));
		window->setThreadId(message.threadId());
		FMessageProcessor->messageToText(window->editWidget()->document(),message);
		window->editWidget()->textEdit()->setFocus();
		window->receiversWidget()->clearSelection();
		updateWindow(window);
	}
}

void NormalMessageHandler::onWindowMenuShowChatDialog()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (FMessageProcessor && window)
		FMessageProcessor->createMessageWindow(window->streamJid(),window->contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
}

void NormalMessageHandler::onWindowMenuSendAsChatMessage()
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(action!=NULL ? (QWidget *)action->data(ADR_WINDOW).toLongLong() : NULL);
	if (window)
	{
		QLineEdit *lneSubject = window->instance()->findChild<QLineEdit *>("lneSubject");
		if (lneSubject)
			lneSubject->setEnabled(!action->isChecked());
	}
}

void NormalMessageHandler::onStatusIconsChanged()
{
	foreach(IMessageNormalWindow *window, FWindows)
		updateWindow(window);
}

void NormalMessageHandler::onAvatarChanged(const Jid &AContactJid)
{
	foreach(IMessageNormalWindow *window, FWindows)
		if (window->contactJid() && AContactJid)
			updateWindow(window);
}

void NormalMessageHandler::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.show!=ABefore.show || AItem.status!=ABefore.status)
	{
		IMessageNormalWindow *window = findWindow(APresence->streamJid(),AItem.itemJid);
		if (window)
			updateWindow(window);
	}
}

void NormalMessageHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList contacts = action->data(ADR_CONTACT_JID).toStringList();
		QStringList groups = action->data(ADR_GROUP).toStringList();
		if (messageShowWindow(MHO_NORMALMESSAGEHANDLER,streams.value(0),Jid::null,Message::Normal,IMessageHandler::SM_SHOW))
		{
			IMessageNormalWindow *window = FMessageWidgets->findNormalWindow(streams.value(0),Jid::null,true);
			if (window)
			{
				for (int i=0; i<streams.count(); i++)
				{
					if (!contacts.at(i).isEmpty())
						window->receiversWidget()->setAddressSelection(streams.at(i),contacts.at(i),true);
					if (!groups.at(i).isEmpty())
						window->receiversWidget()->setGroupSelection(streams.at(i),groups.at(i),true);
				}
			}
		}
	}
}

void NormalMessageHandler::onActiveStreamRemoved(const Jid &AStreamJid)
{
	foreach(IMessageNormalWindow *window, FWindows)
		window->address()->removeAddress(AStreamJid);
}

void NormalMessageHandler::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_SHOWNORMALDIALOG && isSelectionAccepted(indexes))
		{
			QMap<int, QStringList> rolesMap = indexesRolesMap(indexes);
			Jid streamJid = rolesMap.value(RDR_STREAM_JID).value(0);
			if (messageShowWindow(MHO_NORMALMESSAGEHANDLER,streamJid,Jid::null,Message::Normal,IMessageHandler::SM_SHOW))
			{
				IMessageNormalWindow *window = FMessageWidgets->findNormalWindow(streamJid,Jid::null,true);
				if (window)
				{
					for (int i=0; i<rolesMap.value(RDR_STREAM_JID).count(); i++)
					{
						Jid streamJid = rolesMap.value(RDR_STREAM_JID).at(i);
						Jid contactJid = rolesMap.value(RDR_PREP_BARE_JID).at(i);
						QString group = rolesMap.value(RDR_GROUP).at(i);

						if (!contactJid.isEmpty())
							window->receiversWidget()->setAddressSelection(streamJid,contactJid,true);
						if (!group.isEmpty())
							window->receiversWidget()->setGroupSelection(streamJid,group,true);
					}
				}
			}
		}
	}
}

void NormalMessageHandler::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void NormalMessageHandler::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		QMap<int, QStringList> rolesMap = indexesRolesMap(AIndexes);

		Action *action = new Action(AMenu);
		action->setText(tr("Send Message"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_MESSAGE);
		action->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
		action->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
		action->setData(ADR_GROUP,rolesMap.value(RDR_GROUP));
		action->setShortcutId(SCT_ROSTERVIEW_SHOWNORMALDIALOG);
		AMenu->addAction(action,AG_RVCM_NORMALMESSAGEHANDLER,true);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
	}
}

void NormalMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AContext.isEmpty())
	{
		foreach (IMessageNormalWindow *window, FWindows)
		{
			if (!FMessageQueue.value(window).isEmpty() && FMessageQueue.value(window).head().type()==AMessageType)
			{
				IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
				if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
				{
					setMessageStyle(window);
					showStyledMessage(window,FMessageQueue.value(window).head());
				}
			}
		}
	}
}
