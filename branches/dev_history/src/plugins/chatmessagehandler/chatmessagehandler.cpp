#include "chatmessagehandler.h"

#include <QApplication>

#define HISTORY_MESSAGES          10
#define HISTORY_TIME_PAST         5

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

static const QList<int> ChatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

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
			connect(FMessageArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
				SLOT(onArchiveRequestFailed(const QString &, const QString &)));
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
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
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
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

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

bool ChatMessageHandler::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);	Q_UNUSED(AIndex); Q_UNUSED(AEvent);
	return false;
}

bool ChatMessageHandler::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder); Q_UNUSED(AEvent);
	if (AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_MY_RESOURCE)
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
			notify.kinds = ANotifications->notificationKinds(NNT_CHAT_MESSAGE);
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
				updateWindow(window);
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
		window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
		if (window)
		{
			window->infoWidget()->autoUpdateFields();
			window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

			connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
			connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(int, const QVariant &)),
            SLOT(onInfoFieldChanged(int, const QVariant &)), Qt::QueuedConnection);
			connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
			connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));

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

			if (Options::node(OPV_MESSAGES_LOAD_HISTORY).value().toBool())
				showHistory(window);
		}
		else
		{
			window = findWindow(AStreamJid,AContactJid);
		}
	}
	return window;
}

IChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	foreach(IChatWindow *window,FWindows)
		if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
			return window;
	return NULL;
}

void ChatMessageHandler::updateWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (AWindow->instance()->isWindow() && FNotifiedMessages.contains(AWindow))
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
	else if (FStatusIcons)
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
		updateWindow(AWindow);
	}
}

void ChatMessageHandler::showHistory(IChatWindow *AWindow)
{
	if (FMessageArchiver && !FHistoryRequests.values().contains(AWindow))
	{
		IArchiveRequest request;
		request.with = AWindow->contactJid().bare();
		request.order = Qt::DescendingOrder;

		WindowStatus &wstatus = FWindowStatus[AWindow];
		if (wstatus.createTime.secsTo(QDateTime::currentDateTime()) < HISTORY_TIME_PAST)
		{
			request.maxItems = HISTORY_MESSAGES;
			request.end = QDateTime::currentDateTime().addSecs(-HISTORY_TIME_PAST);
		}
		else
		{
			request.start = wstatus.startTime.isValid() ? wstatus.startTime : wstatus.createTime;
			request.end = QDateTime::currentDateTime();
		}

		QString reqId = FMessageArchiver->loadMessages(AWindow->streamJid(),request);
		if (!reqId.isEmpty())
		{
			showStyledStatus(AWindow,tr("Loading history..."),false);
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
			AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().node());
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

void ChatMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage, bool AArchive)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::KindStatus;
	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;

	if (AArchive && FMessageArchiver && Options::node(OPV_MESSAGES_ARCHIVESTATUS).value().toBool())
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

	if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid()!=AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionOut;

	if (options.time.secsTo(FWindowStatus.value(AWindow).createTime)>HISTORY_TIME_PAST)
		options.type |= IMessageContentOptions::TypeHistory;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,options.time);
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

void ChatMessageHandler::onMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (FMessageProcessor && window)
	{
		Message message;
		message.setTo(window->contactJid().eFull()).setType(Message::Chat);
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			if (FMessageProcessor->sendMessage(window->streamJid(),message,IMessageProcessor::MessageOut))
				window->editWidget()->clearEditor();
		}
	}
}

void ChatMessageHandler::onInfoFieldChanged(int AField, const QVariant &AValue)
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
		if (AId == SCT_ROSTERVIEW_SHOWCHATDIALOG)
		{
			QModelIndex index = FRostersView->instance()->currentIndex();
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(index.data(RDR_STREAM_JID).toString()) : NULL;
			if (presence && presence->isOpen() && ChatActionTypes.contains(index.data(RDR_TYPE).toInt()))
			{
				messageShowWindow(MHO_CHATMESSAGEHANDLER,index.data(RDR_STREAM_JID).toString(),index.data(RDR_FULL_JID).toString(),Message::Chat,IMessageHandler::SM_SHOW);
			}
		}
	}
}

void ChatMessageHandler::onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		setMessageStyle(window);

		int messageIt = ABody.messages.count()-1;
		QMultiMap<QDateTime,QString>::const_iterator noteIt = ABody.notes.constBegin();
		while (messageIt>=0 || noteIt!=ABody.notes.constEnd())
		{
			if (messageIt>=0 && (noteIt==ABody.notes.constEnd() || ABody.messages.at(messageIt).dateTime()<noteIt.key()))
			{
				showStyledMessage(window,ABody.messages.at(messageIt));
				messageIt--;
			}
			else if (noteIt != ABody.notes.constEnd())
			{
				showStyledStatus(window,noteIt.value(),false);
				noteIt++;
			}
		}

		foreach(Message message, FPendingMessages.take(window))
			showStyledMessage(window,message);

		WindowStatus &wstatus = FWindowStatus[window];
		wstatus.startTime = ABody.messages.value(0).dateTime();
	}
}

void ChatMessageHandler::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		showStyledStatus(window,tr("Failed to load history: %1").arg(AError),false);
		FPendingMessages.remove(window);
	}
}

void ChatMessageHandler::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if (ALabelId==RLID_DISPLAY && AIndexes.count()==1)
	{
		Jid streamJid = AIndexes.first()->data(RDR_STREAM_JID).toString();
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
		if (presence && presence->isOpen())
		{
			Jid contactJid = AIndexes.first()->data(RDR_FULL_JID).toString();
			if (ChatActionTypes.contains(AIndexes.first()->type()))
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Open chat dialog"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
				action->setData(ADR_STREAM_JID,streamJid.full());
				action->setData(ADR_CONTACT_JID,contactJid.full());
				action->setShortcutId(SCT_ROSTERVIEW_SHOWCHATDIALOG);
				AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER,true);
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
			}
		}
	}
}

void ChatMessageHandler::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (!AItem.itemJid.resource().isEmpty() && AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error)
	{
		IChatWindow *fullWindow = findWindow(APresence->streamJid(),AItem.itemJid);
		
		IChatWindow *bareWindow = findWindow(APresence->streamJid(),AItem.itemJid.bare());
		if (bareWindow)
		{
			if (!fullWindow)
				bareWindow->setContactJid(AItem.itemJid);
			else if (!FNotifiedMessages.contains(bareWindow))
				bareWindow->instance()->deleteLater();
		}

		if (!fullWindow && !bareWindow)
		{
			foreach(IChatWindow *offlineWindow, FWindows)
			{
				if (offlineWindow->streamJid()==APresence->streamJid() && (offlineWindow->contactJid() && AItem.itemJid))
				{
					int show = APresence->presenceItem(offlineWindow->contactJid()).show;
					if (show==IPresence::Offline || show==IPresence::Error)
					{
						offlineWindow->setContactJid(AItem.itemJid);
						break;
					}
				}
			}
		}
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