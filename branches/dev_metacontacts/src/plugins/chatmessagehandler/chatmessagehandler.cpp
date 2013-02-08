#include "chatmessagehandler.h"

#include <QMouseEvent>
#include <QApplication>

#define HISTORY_MESSAGES          10
#define HISTORY_TIME_DELTA        5
#define HISTORY_DUBLICATE_DELTA   2*60

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

ChatMessageHandler::ChatMessageHandler()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FPresencePlugin = NULL;
	FMessageArchiver = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FXmppUriQueries = NULL;
	FOptionsManager = NULL;
	FRecentContacts = NULL;
}

ChatMessageHandler::~ChatMessageHandler()
{

}

void ChatMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Chat Messages");
	APluginInfo->description = tr("Allows to exchange chat messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}


bool ChatMessageHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

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

	plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
		if (FMessageArchiver)
		{
			connect(FMessageArchiver->instance(),SIGNAL(messagesLoaded(const QString &, const IArchiveCollectionBody &)),
				SLOT(onArchiveMessagesLoaded(const QString &, const IArchiveCollectionBody &)));
			connect(FMessageArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
				SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
		if (notifications)
		{
			INotificationType notifyType;
			notifyType.order = NTO_CHATHANDLER_MESSAGE;
			notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
			notifyType.title = tr("When receiving new chat message");
			notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
			notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
			notifications->registerNotificationType(NNT_CHAT_MESSAGE,notifyType);
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

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

bool ChatMessageHandler::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW, tr("Clear window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWCHATDIALOG, tr("Open chat dialog"), tr("Return","Open chat dialog"), Shortcuts::WidgetShortcut);

	if (FRostersView)
	{
		FRostersView->insertClickHooker(RCHO_CHATMESSAGEHANDLER,this);
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWCHATDIALOG,FRostersView->instance());
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_CHATMESSAGEHANDLER,this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	return true;
}

bool ChatMessageHandler::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_LOAD_HISTORY, true);
	if (FOptionsManager)
		FOptionsManager->insertOptionsHolder(this);
	return true;
}

QMultiMap<int, IOptionsWidget *> ChatMessageHandler::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES_LOADHISTORY,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_LOAD_HISTORY),tr("Load messages from history in new chat windows"),AParent));
	}
	return widgets;
}

bool ChatMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type == "chat")
		{
			IChatWindow *window = getWindow(AStreamJid, AContactJid);
			window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
			window->showTabPage();
			return true;
		}
	}
	return false;
}

bool ChatMessageHandler::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		return rosterIndexDoubleClicked(AOrder, AIndex, AEvent);
	return false;
}

bool ChatMessageHandler::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);
	if (AEvent->modifiers()==Qt::NoModifier && (AIndex->kind()==RIK_CONTACT || AIndex->kind()==RIK_MY_RESOURCE))
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		return messageShowWindow(MHO_CHATMESSAGEHANDLER,streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
	return false;
}

bool ChatMessageHandler::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder); Q_UNUSED(ADirection);
	return AMessage.type()==Message::Chat && !AMessage.body().isEmpty();
}

bool ChatMessageHandler::messageDisplay(const Message &AMessage, int ADirection)
{
	IChatWindow *window = NULL;
	if (ADirection == IMessageProcessor::MessageIn)
		window = AMessage.type()!=Message::Error ? getWindow(AMessage.to(),AMessage.from()) : findWindow(AMessage.to(),AMessage.from());
	else
		window = AMessage.type()!=Message::Error ? getWindow(AMessage.from(),AMessage.to()) : findWindow(AMessage.from(),AMessage.to());
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
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		if (FHistoryRequests.values().contains(window))
			FPendingMessages[window].append(AMessage);
		showStyledMessage(window,AMessage);
	}

	return window!=NULL;
}

INotification ChatMessageHandler::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::MessageIn)
	{
		IChatWindow *window = findWindow(AMessage.to(),AMessage.from());
		if (window && !window->isActiveTabPage())
		{
			notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_CHAT_MESSAGE);
			if (notify.kinds > 0)
			{
				QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
				QString name = ANotifications->contactName(AMessage.to(),AMessage.from());

				notify.typeId = NNT_CHAT_MESSAGE;
				notify.data.insert(NDR_ICON,icon);
				notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
				notify.data.insert(NDR_STREAM_JID,AMessage.to());
				notify.data.insert(NDR_CONTACT_JID,AMessage.from());
				notify.data.insert(NDR_ROSTER_ORDER,RNO_CHATMESSAGE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
				notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.from()));
				notify.data.insert(NDR_POPUP_CAPTION, tr("Message received"));
				notify.data.insert(NDR_POPUP_TITLE,name);
				notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);

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
					notify.data.insert(NDR_POPUP_HTML,Qt::escape(AMessage.body()));
				}

				FNotifiedMessages.insertMulti(window, AMessage.data(MDR_MESSAGE_ID).toInt());
			}
		}
	}
	return notify;
}

bool ChatMessageHandler::messageShowWindow(int AMessageId)
{
	IChatWindow *window = FNotifiedMessages.key(AMessageId);
	if (window)
	{
		window->showTabPage();
		return true;
	}
	return false;
}

bool ChatMessageHandler::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType == Message::Chat)
	{
		IChatWindow *window = getWindow(AStreamJid,AContactJid);
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

IChatWindow *ChatMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = NULL;
	if (AStreamJid.isValid() && AContactJid.isValid())
	{
		window = findSubstituteWindow(AStreamJid,AContactJid);
		if (!window)
		{
			window = FMessageWidgets->getChatWindow(AStreamJid,AContactJid);
			if (window)
			{
				window->infoWidget()->autoUpdateFields();
				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));
				connect(window->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onWindowNotifierActiveNotifyChanged(int)));
				connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(int, const QVariant &)),SLOT(onWindowInfoFieldChanged(int, const QVariant &)), Qt::QueuedConnection);

				FWindows.append(window);
				FWindowStatus[window].createTime = QDateTime::currentDateTime();
				updateWindow(window);
				setMessageStyle(window);

				Action *clearAction = new Action(window->instance());
				clearAction->setText(tr("Clear Chat Window"));
				clearAction->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_CLEAR_CHAT);
				clearAction->setShortcutId(SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW);
				connect(clearAction,SIGNAL(triggered(bool)),SLOT(onClearWindowAction(bool)));
				window->toolBarWidget()->toolBarChanger()->insertAction(clearAction, TBG_MWTBW_CLEAR_WINDOW);

				if (FRostersView && FRostersModel)
				{
					UserContextMenu *menu = new UserContextMenu(FRostersModel,FRostersView,window);
					menu->menuAction()->setIcon(RSR_STORAGE_MENUICONS, MNI_CHAT_MHANDLER_USER_MENU);
					QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
					button->setPopupMode(QToolButton::InstantPopup);
				}

				showHistory(window);
			}
			else
			{
				window = findWindow(AStreamJid,AContactJid);
			}
		}
		else if(!AContactJid.resource().isEmpty() && window->contactJid()!=AContactJid)
		{
			window->setContactJid(AContactJid);
		}
	}
	return window;
}

IChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IChatWindow *window, FWindows)
		if (window->streamJid()==AStreamJid && window->contactJid()==AContactJid)
			return window;
	return NULL;
}

IChatWindow *ChatMessageHandler::findSubstituteWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	IChatWindow *fullWindow = NULL;
	IChatWindow *bareWindow = NULL;
	IChatWindow *offlineWindow = NULL;
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;

	int maxResDiff = -1;
	foreach(IChatWindow *window, FWindows)
	{
		if (window->streamJid() == AStreamJid)
		{
			if (window->contactJid() == AContactJid)
			{
				fullWindow = window;
				break;
			}
			else if(presence && !bareWindow && (window->contactJid() && AContactJid))
			{
				IPresenceItem pitem = presence->presenceItem(window->contactJid());
				if (pitem.show==IPresence::Offline || pitem.show==IPresence::Error)
				{
					if (window->contactJid() == AContactJid.bare())
					{
						bareWindow = window;
					}
					else
					{
						int resDiff = 0;
						QString contactRes = AContactJid.resource();
						QString offlineRes = window->contactJid().resource();
						while(offlineRes.size()>resDiff && contactRes.size()>resDiff && offlineRes.at(resDiff)==contactRes.at(resDiff))
						{
							resDiff++;
						}
						if (maxResDiff < resDiff)
						{
							maxResDiff = resDiff;
							offlineWindow = window;
						}
					}
				}
			}
		}
	}

	if (fullWindow)
		return fullWindow;
	else if(bareWindow)
		return bareWindow;
	else if(offlineWindow)
		return offlineWindow;

	return NULL;
}

void ChatMessageHandler::updateWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
		icon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;
	if (FStatusIcons && icon.isNull())
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
	AWindow->updateWindow(icon,contactName,tr("%1 - Chat").arg(contactName),QString::null);
}

void ChatMessageHandler::removeNotifiedMessages(IChatWindow *AWindow)
{
	if (FNotifiedMessages.contains(AWindow))
	{
		foreach(int messageId, FNotifiedMessages.values(AWindow))
			FMessageProcessor->removeMessageNotify(messageId);
		FNotifiedMessages.remove(AWindow);
	}
}

void ChatMessageHandler::showHistory(IChatWindow *AWindow)
{
	if (FMessageArchiver && Options::node(OPV_MESSAGES_LOAD_HISTORY).value().toBool() && !FHistoryRequests.values().contains(AWindow))
	{
		WindowStatus &wstatus = FWindowStatus[AWindow];

		IArchiveRequest request;
		request.with = AWindow->contactJid().bare();
		request.exactmatch = request.with.node().isEmpty();
		request.order = Qt::DescendingOrder;
		if (wstatus.createTime.secsTo(QDateTime::currentDateTime()) > HISTORY_TIME_DELTA)
			request.start = wstatus.startTime.isValid() ? wstatus.startTime : wstatus.createTime;
		else
			request.maxItems = HISTORY_MESSAGES;
		request.end = QDateTime::currentDateTime();

		QString reqId = FMessageArchiver->loadMessages(AWindow->streamJid(),request);
		if (!reqId.isEmpty())
		{
			showStyledStatus(AWindow,tr("Loading history..."),true);
			FHistoryRequests.insert(reqId,AWindow);
		}
	}
}

void ChatMessageHandler::setMessageStyle(IChatWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
	if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true))
	{
		IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
		AWindow->viewWidget()->setMessageStyle(style,soptions);
	}
	FWindowStatus[AWindow].lastDateSeparator = QDate();
}

void ChatMessageHandler::fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
	if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		AOptions.senderId = AWindow->contactJid().full();
		AOptions.senderName = Qt::escape(FMessageStyles->contactName(AWindow->streamJid(),AWindow->contactJid()));
		AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->contactJid());
		AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid(),AWindow->contactJid());
		AOptions.senderColor = "blue";
	}
	else
	{
		AOptions.senderId = AWindow->streamJid().full();
		if (AWindow->streamJid() && AWindow->contactJid())
			AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().uNode());
		else
			AOptions.senderName = Qt::escape(FMessageStyles->contactName(AWindow->streamJid()));
		AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->streamJid());
		AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid());
		AOptions.senderColor = "red";
	}
}

void ChatMessageHandler::showDateSeparator(IChatWindow *AWindow, const QDateTime &ADateTime)
{
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
	{
		QDate sepDate = ADateTime.date();
		WindowStatus &wstatus = FWindowStatus[AWindow];
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
			AWindow->viewWidget()->appendText(FMessageStyles->dateSeparator(sepDate),options);
		}
	}
}

void ChatMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage, bool ADontSave, const QDateTime &ATime)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindStatus;
	options.direction = IMessageContentOptions::DirectionIn;

	options.time = ATime;
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyles->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (!ADontSave && FMessageArchiver && Options::node(OPV_MESSAGES_ARCHIVESTATUS).value().toBool())
		FMessageArchiver->saveNote(AWindow->streamJid(), AWindow->contactJid(), AMessage);

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,options.time);
	AWindow->viewWidget()->appendText(AMessage,options);
}

void ChatMessageHandler::showStyledMessage(IChatWindow *AWindow, const Message &AMessage)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindMessage;

	options.time = AMessage.dateTime();
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyles->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (options.time.secsTo(FWindowStatus.value(AWindow).createTime)>HISTORY_TIME_DELTA)
		options.type |= IMessageContentOptions::TypeHistory;

	if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid()!=AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionOut;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,options.time);
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

bool ChatMessageHandler::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> chatDialogKinds = QList<int>() << RIK_CONTACT << RIK_AGENT << RIK_MY_RESOURCE;
	if (!ASelected.isEmpty())
	{
		foreach(IRosterIndex *index, ASelected)
		{
			int indexKinds = index->kind();
			if (!chatDialogKinds.contains(indexKinds))
				return false;
		}
		return true;
	}
	return false;
}

void ChatMessageHandler::onMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (FMessageProcessor && window)
	{
		Message message;
		message.setTo(window->contactJid().full()).setType(Message::Chat);
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			if (FMessageProcessor->sendMessage(window->streamJid(),message,IMessageProcessor::MessageOut))
				window->editWidget()->clearEditor();
		}
	}
}

void ChatMessageHandler::onWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		removeNotifiedMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
	}
}

void ChatMessageHandler::onWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		int destroyTimeout = Options::node(OPV_MESSAGES_CLEANCHATTIMEOUT).value().toInt();
		if (destroyTimeout>0 && !FNotifiedMessages.contains(window))
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
}

void ChatMessageHandler::onWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (FWindows.contains(window))
	{
		removeNotifiedMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		FWindows.removeAt(FWindows.indexOf(window));
		FWindowStatus.remove(window);
		FPendingMessages.remove(window);
		FHistoryRequests.remove(FHistoryRequests.key(window));
	}
}

void ChatMessageHandler::onWindowNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	IChatWindow *window = notifier!=NULL ? qobject_cast<IChatWindow *>(notifier->tabPage()->instance()) : NULL;
	if (window)
		updateWindow(window);
}

void ChatMessageHandler::onWindowInfoFieldChanged(int AField, const QVariant &AValue)
{
	Q_UNUSED(AValue);
	if (AField==IInfoWidget::ContactShow || AField==IInfoWidget::ContactStatus || AField==IInfoWidget::ContactName)
	{
		IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			if (AField==IInfoWidget::ContactShow || AField==IInfoWidget::ContactStatus)
			{
				QString status = widget->field(IInfoWidget::ContactStatus).toString();
				QString show = FStatusChanger ? FStatusChanger->nameByShow(widget->field(IInfoWidget::ContactShow).toInt()) : QString::null;
				WindowStatus &wstatus = FWindowStatus[window];
				if (Options::node(OPV_MESSAGES_SHOWSTATUS).value().toBool() && wstatus.lastStatusShow!=status+show)
				{
					QString message = tr("%1 changed status to [%2] %3").arg(widget->field(IInfoWidget::ContactName).toString()).arg(show).arg(status);
					showStyledStatus(window,message);
				}
				wstatus.lastStatusShow = status+show;
			}
			updateWindow(window);
		}
	}
}

void ChatMessageHandler::onStatusIconsChanged()
{
	foreach(IChatWindow *window, FWindows)
		updateWindow(window);
}

void ChatMessageHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		messageShowWindow(MHO_CHATMESSAGEHANDLER,streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
}

void ChatMessageHandler::onClearWindowAction(bool)
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

void ChatMessageHandler::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance() && !FRostersView->hasMultiSelection())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_SHOWCHATDIALOG && isSelectionAccepted(indexes))
		{
			IRosterIndex *index = indexes.first();
			messageShowWindow(MHO_CHATMESSAGEHANDLER,index->data(RDR_STREAM_JID).toString(),index->data(RDR_FULL_JID).toString(),Message::Chat,IMessageHandler::SM_SHOW);
		}
	}
}

void ChatMessageHandler::onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		setMessageStyle(window);

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
				showStyledMessage(window,ABody.messages.at(messageIt));
				messageIt--;
			}
			else if (noteIt != ABody.notes.constEnd())
			{
				showStyledStatus(window,noteIt.value(),true,noteIt.key());
				++noteIt;
			}
		}

		foreach(Message message, pendingMessages)
			showStyledMessage(window,message);

		WindowStatus &wstatus = FWindowStatus[window];
		wstatus.startTime = !ABody.messages.isEmpty() ? ABody.messages.last().dateTime() : QDateTime();
	}
}

void ChatMessageHandler::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		showStyledStatus(window,tr("Failed to load history: %1").arg(AError.errorMessage()),true);
		FPendingMessages.remove(window);
	}
}

void ChatMessageHandler::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		if (!FRostersView->hasMultiSelection())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Open chat dialog"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,AIndexes.first()->data(RDR_FULL_JID));
			action->setShortcutId(SCT_ROSTERVIEW_SHOWCHATDIALOG);
			AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void ChatMessageHandler::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (!AItem.itemJid.resource().isEmpty() && AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error && (ABefore.show==IPresence::Offline || ABefore.show==IPresence::Error))
	{
		IChatWindow *window = findSubstituteWindow(APresence->streamJid(),AItem.itemJid);
		if (window && window->contactJid()!=AItem.itemJid)
			window->setContactJid(AItem.itemJid);
	}
}

void ChatMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IChatWindow *window, FWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setMessageStyle(window);
				showHistory(window);
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_chatmessagehandler, ChatMessageHandler)
