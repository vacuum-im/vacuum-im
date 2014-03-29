#include "chatmessagehandler.h"

#include <QMouseEvent>
#include <QApplication>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/recentitemtypes.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/messageeditsendhandlerorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/logger.h>

#define HISTORY_MESSAGES          10
#define HISTORY_TIME_DELTA        5
#define HISTORY_DUBLICATE_DELTA   2*60

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

ChatMessageHandler::ChatMessageHandler()
{
	FAvatars = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FMessageArchiver = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FNotifications = NULL;
	FAccountManager = NULL;
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
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveStreamRemoved(const Jid &)));
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

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
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

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRecentContacts").value(0,NULL);
	if (plugin)
	{
		FRecentContacts = qobject_cast<IRecentContacts *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool ChatMessageHandler::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW, tr("Clear window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWCHATDIALOG, tr("Open chat dialog"), tr("Return","Open chat dialog"), Shortcuts::WidgetShortcut);

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_CHATHANDLER_MESSAGE;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATMHANDLER_MESSAGE);
		notifyType.title = tr("When receiving new chat message");
		notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_CHAT_MESSAGE,notifyType);
	}
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
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	if (FMessageWidgets)
	{
		FMessageWidgets->insertEditSendHandler(MESHO_CHATMESSAGEHANDLER,this);
	}
	return true;
}

bool ChatMessageHandler::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_LOAD_HISTORY, true);
	return true;
}

bool ChatMessageHandler::messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget)
{
	Q_UNUSED(AOrder); Q_UNUSED(AWidget);
	return false;
}

bool ChatMessageHandler::messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget)
{
	if (AOrder == MESHO_CHATMESSAGEHANDLER)
	{
		IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(AWidget->messageWindow()->instance());
		if (FMessageProcessor && FWindows.contains(window))
		{
			Message message;
			message.setTo(window->contactJid().full()).setType(Message::Chat);
			FMessageProcessor->textToMessage(message,AWidget->document());
			return !message.body().isEmpty() && FMessageProcessor->sendMessage(window->streamJid(),message,IMessageProcessor::MessageOut);
		}
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
	IMessageChatWindow *window = ADirection==IMessageProcessor::MessageIn ? getWindow(AMessage.to(),AMessage.from()) : getWindow(AMessage.from(),AMessage.to());
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

		if (ADirection == IMessageProcessor::MessageIn)
		{
			if (window->streamJid()!=AMessage.to() || window->contactJid()!=AMessage.from())
			{
				LOG_STRM_DEBUG(window->streamJid(),QString("Changing chat window address from=%1 to=%2").arg(window->contactJid().full(),AMessage.from()));
				window->address()->setAddress(AMessage.to(),AMessage.from());
			}
		}

		showStyledMessage(window,AMessage);
	}
	else
	{
		REPORT_ERROR(QString("Failed to display message type=%1: Chat window not created").arg(AMessage.type()));
	}

	return window!=NULL;
}

INotification ChatMessageHandler::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::MessageIn)
	{
		IMessageChatWindow *window = findWindow(AMessage.to(),AMessage.from());
		if (window && !window->isActiveTabPage())
		{
			notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_CHAT_MESSAGE);
			if (notify.kinds > 0)
			{
				QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATMHANDLER_MESSAGE);
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
				notify.data.insert(NDR_POPUP_TEXT,AMessage.body());

				FNotifiedMessages.insertMulti(window, AMessage.data(MDR_MESSAGE_ID).toInt());
			}
		}
		else if (window == NULL)
		{
			LOG_STRM_ERROR(AMessage.to(),QString("Failed to notify message from=%1: Window not found").arg(AMessage.from()));
		}
	}
	return notify;
}

bool ChatMessageHandler::messageShowWindow(int AMessageId)
{
	IMessageChatWindow *window = FNotifiedMessages.key(AMessageId);
	if (window)
	{
		window->showTabPage();
		return true;
	}
	else
	{
		REPORT_ERROR("Failed to show notified chat message window: Window not found");
	}
	return false;
}

bool ChatMessageHandler::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType == Message::Chat)
	{
		IMessageChatWindow *window = getWindow(AStreamJid,AContactJid);
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
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to show chat message window, with=%1: Window not created").arg(AContactJid.bare()));
		}
	}
	return false;
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

bool ChatMessageHandler::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		return rosterIndexDoubleClicked(AOrder, AIndex, AEvent);
	return false;
}

bool ChatMessageHandler::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (AOrder==RCHO_CHATMESSAGEHANDLER && AEvent->modifiers()==Qt::NoModifier && (AIndex->kind()==RIK_CONTACT || AIndex->kind()==RIK_MY_RESOURCE))
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		return messageShowWindow(MHO_CHATMESSAGEHANDLER,streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
	return false;
}

bool ChatMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type == "chat")
		{
			IMessageChatWindow *window = getWindow(AStreamJid, AContactJid);
			if (window)
			{
				window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
				window->showTabPage();
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to open chat window by XMPP URI, with=%1: Window not created").arg(AContactJid.bare()));
			}
		}
	}
	return false;
}

IMessageChatWindow *ChatMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IMessageChatWindow *window = NULL;
	if (FMessageProcessor && FMessageProcessor->isActiveStream(AStreamJid) && AContactJid.isValid())
	{
		window = findWindow(AStreamJid,AContactJid);
		if (!window)
		{
			window = FMessageWidgets->getChatWindow(AStreamJid,AContactJid);
			if (window)
			{
				LOG_STRM_INFO(AStreamJid,QString("Chat window created, with=%1").arg(AContactJid.bare()));

				window->address()->setAutoAddresses(true);
				window->infoWidget()->setAddressMenuVisible(true);
				window->infoWidget()->addressMenu()->menuAction()->setToolTip(tr("Contact resource"));
				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));
				connect(window->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onWindowAddressChanged()));
				connect(window->address()->instance(),SIGNAL(availAddressesChanged()),SLOT(onWindowAvailAddressesChanged()));
				connect(window->infoWidget()->instance(),SIGNAL(addressMenuRequested(Menu *)),SLOT(onWindowAddressMenuRequested(Menu *)));
				connect(window->infoWidget()->instance(),SIGNAL(contextMenuRequested(Menu *)),SLOT(onWindowContextMenuRequested(Menu *)));
				connect(window->infoWidget()->instance(),SIGNAL(toolTipsRequested(QMap<int,QString> &)),SLOT(onWindowToolTipsRequested(QMap<int,QString> &)));
				connect(window->viewWidget()->instance(),SIGNAL(contentAppended(const QString &, const IMessageContentOptions &)),
					SLOT(onWindowContentAppended(const QString &, const IMessageContentOptions &)));
				connect(window->viewWidget()->instance(),SIGNAL(messageStyleOptionsChanged(const IMessageStyleOptions &, bool)),
					SLOT(onWindowMessageStyleOptionsChanged(const IMessageStyleOptions &, bool)));
				connect(window->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onWindowNotifierActiveNotifyChanged(int)));

				FWindows.append(window);
				FWindowStatus[window].createTime = QDateTime::currentDateTime();

				Action *clearAction = new Action(window->instance());
				clearAction->setText(tr("Clear Chat Window"));
				clearAction->setIcon(RSR_STORAGE_MENUICONS,MNI_CHATMHANDLER_CLEAR_CHAT);
				clearAction->setShortcutId(SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW);
				connect(clearAction,SIGNAL(triggered(bool)),SLOT(onClearWindowAction(bool)));
				window->toolBarWidget()->toolBarChanger()->insertAction(clearAction, TBG_MWTBW_CLEAR_WINDOW);

				updateWindow(window);
				setMessageStyle(window);
				showHistory(window);
			}
			else
			{
				LOG_STRM_ERROR(AStreamJid,QString("Failed to create chat window, with=%1: Instance is not created").arg(AContactJid.bare()));
			}
		}
	}
	else if (FMessageProcessor == NULL)
	{
		REPORT_ERROR("Failed to create chat window: IMessageProcessor is NULL");
	}
	else if (!FMessageProcessor->isActiveStream(AStreamJid))
	{
		REPORT_ERROR("Failed to create chat window: Stream is not active");
	}
	else if (!AContactJid.isValid())
	{
		REPORT_ERROR("Failed to create chat window: Contact is not valid");
	}
	return window;
}

IMessageChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach(IMessageChatWindow *window, FWindows)
		if (window->address()->availAddresses(true).contains(AStreamJid,AContactJid.bare()))
			return window;
	return NULL;
}

void ChatMessageHandler::updateWindow(IMessageChatWindow *AWindow)
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
	
	QIcon statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid()) : QIcon();
	AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);

	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AWindow->streamJid()) : NULL;
	IPresenceItem pitem = presence!=NULL ? presence->findItem(AWindow->contactJid()) : IPresenceItem();
	AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusText,pitem.status);

	QString resource = !AWindow->contactJid().resource().isEmpty() ? AWindow->contactJid().resource() : tr("<Absent>");
	AWindow->infoWidget()->addressMenu()->setTitle(TextManager::getElidedString(resource,Qt::ElideRight,20));

	QIcon tabIcon = statusIcon;
	if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
		tabIcon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;

	AWindow->updateWindow(tabIcon,name,tr("%1 - Chat").arg(name),QString::null);
}

void ChatMessageHandler::removeNotifiedMessages(IMessageChatWindow *AWindow)
{
	if (FNotifiedMessages.contains(AWindow))
	{
		foreach(int messageId, FNotifiedMessages.values(AWindow))
			FMessageProcessor->removeMessageNotify(messageId);
		FNotifiedMessages.remove(AWindow);
	}
}

void ChatMessageHandler::showHistory(IMessageChatWindow *AWindow)
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
			LOG_STRM_INFO(AWindow->streamJid(),QString("Load chat history request sent, with=%1, id=%2").arg(request.with.bare(),reqId));
			showStyledStatus(AWindow,tr("Loading history..."),true);
			FHistoryRequests.insert(reqId,AWindow);
		}
		else
		{
			LOG_STRM_WARNING(AWindow->streamJid(),QString("Failed to send chat history load request, with=%1").arg(request.with.bare()));
		}
	}
}

void ChatMessageHandler::setMessageStyle(IMessageChatWindow *AWindow)
{
	if (FMessageStyles)
	{
		LOG_STRM_DEBUG(AWindow->streamJid(),QString("Changing message style for chat window, with=%1").arg(AWindow->contactJid().bare()));
		IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
		if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true))
		{
			IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
			AWindow->viewWidget()->setMessageStyle(style,soptions);
		}
		FWindowStatus[AWindow].lastDateSeparator = QDate();
	}
}

void ChatMessageHandler::fillContentOptions(IMessageChatWindow *AWindow, IMessageContentOptions &AOptions) const
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

void ChatMessageHandler::showDateSeparator(IMessageChatWindow *AWindow, const QDateTime &ADateTime)
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

void ChatMessageHandler::showStyledStatus(IMessageChatWindow *AWindow, const QString &AMessage, bool ADontSave, const QDateTime &ATime)
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

void ChatMessageHandler::showStyledMessage(IMessageChatWindow *AWindow, const Message &AMessage)
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

	if (AWindow->streamJid()==AMessage.to() || AMessage.to().isEmpty())
		options.direction = IMessageContentOptions::DirectionIn;
	else if (AWindow->streamJid()==AMessage.from() || AMessage.from().isEmpty())
		options.direction = IMessageContentOptions::DirectionOut;
	else if (AWindow->contactJid() == AMessage.to())
		options.direction = IMessageContentOptions::DirectionOut;
	else if (AWindow->contactJid() == AMessage.from())
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionIn;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,options.time);
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

bool ChatMessageHandler::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> chatDialogKinds = QList<int>() << RIK_CONTACT << RIK_AGENT << RIK_MY_RESOURCE;
	foreach(IRosterIndex *index, ASelected)
	{
		int indexKinds = index->kind();
		if (!chatDialogKinds.contains(indexKinds))
			return false;
	}
	return !ASelected.isEmpty();
}

QMap<Jid, QList<Jid> > ChatMessageHandler::getSortedAddresses(const QMultiMap<Jid,Jid> &AAddresses) const
{
	QMap<Jid, QList<Jid> > addresses;
	foreach(const Jid &streamJid, AAddresses.uniqueKeys())
	{
		QList<Jid> contacts = AAddresses.values(streamJid);
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
		if (presence)
		{
			QList<IPresenceItem> pitemList;
			foreach(const Jid &contactJid, contacts)
			{
				IPresenceItem pitem = presence->findItem(contactJid);
				pitem.itemJid = contactJid;
				pitemList.append(pitem);
			}

			contacts.clear();
			pitemList = FPresencePlugin->sortPresenceItems(pitemList);
			foreach(const IPresenceItem &pitem, pitemList)
				contacts.append(pitem.itemJid);
		}
		else
		{
			REPORT_ERROR("Failed to sort addresses: IPresence not found");
		}
		addresses[streamJid] = contacts;
	}
	return addresses;
}

void ChatMessageHandler::onWindowActivated()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		LOG_STRM_DEBUG(window->streamJid(),QString("Chat window activated, with=%1").arg(window->contactJid().bare()));
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		removeNotifiedMessages(window);
	}
}

void ChatMessageHandler::onWindowClosed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		LOG_STRM_DEBUG(window->streamJid(),QString("Chat window closed, with=%1").arg(window->contactJid().bare()));
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
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (FWindows.contains(window))
	{
		LOG_STRM_INFO(window->streamJid(),QString("Chat window destroyed, with=%1").arg(window->contactJid().bare()));
		removeNotifiedMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		FWindows.removeAt(FWindows.indexOf(window));
		FWindowStatus.remove(window);
		FPendingMessages.remove(window);
		FPendingContent.remove(window);
		FHistoryRequests.remove(FHistoryRequests.key(window));
	}
}

void ChatMessageHandler::onWindowAddressChanged()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender()->parent());
	if (window)
	{
		LOG_STRM_DEBUG(window->streamJid(),QString("Chat window address changed, with=%1").arg(window->contactJid().bare()));
		updateWindow(window);
	}
}

void ChatMessageHandler::onWindowAvailAddressesChanged()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender()->parent());
	if (window)
	{
		QMultiMap<Jid,Jid> addresses = window->address()->availAddresses();
		if (addresses.isEmpty())
		{
			LOG_STRM_DEBUG(window->streamJid(),QString("Destroying chat window due to avail addresses is empty, with=%1").arg(window->contactJid().bare()));
			window->instance()->deleteLater();
		}
		else if (!addresses.contains(window->streamJid(),window->contactJid()))
		{
			LOG_STRM_DEBUG(window->streamJid(),QString("Changing chat window address due to avail addresses changed, with=%1").arg(window->contactJid().bare()));
			QMap<Jid, QList<Jid> > sortedAddresses = getSortedAddresses(addresses);
			QMap<Jid, QList<Jid> >::const_iterator it = sortedAddresses.constBegin();
			window->address()->setAddress(it.key(),it.value().first());
		}
		else
		{
			LOG_STRM_DEBUG(window->streamJid(),QString("Chat window avail addresses changed, with=%1").arg(window->contactJid().bare()));
		}
	}
}

void ChatMessageHandler::onWindowAddressMenuRequested(Menu *AMenu)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	if (widget)
	{
		QMap<Jid, QList<Jid> > addresses = getSortedAddresses(widget->messageWindow()->address()->availAddresses());

		Jid lastStreamJid;
		int streamGroup = AG_MWIWAM_CHATMHANDLER_ADDRESSES-1;
		foreach(const Jid &streamJid, addresses.keys())
		{
			IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(streamJid) : NULL;
			QString accountName = account!=NULL ? account->name() : streamJid.uBare();

			streamGroup++;
			Action *accountAction = new Action(AMenu);
			accountAction->setText(QString("<%1>").arg(accountName));
			accountAction->setEnabled(false);
			QFont font = accountAction->font();
			font.setBold(true);
			accountAction->setFont(font);
			AMenu->addAction(accountAction,streamGroup);

			foreach(const Jid &contactJid, addresses.value(streamJid))
			{
				QString addressName = FMessageStyles!=NULL ? FMessageStyles->contactName(streamJid,contactJid) : contactJid.uBare();
				if (!contactJid.resource().isEmpty() && addressName!=contactJid.resource())
					addressName += "/" + contactJid.resource();

				bool isCurAddress = streamJid==widget->messageWindow()->streamJid() && contactJid==widget->messageWindow()->contactJid();

				Action *addressAction = new Action(AMenu);
				addressAction->setCheckable(true);
				addressAction->setChecked(isCurAddress);
				addressAction->setText(addressName);
				addressAction->setData(ADR_STREAM_JID,streamJid.full());
				addressAction->setData(ADR_CONTACT_JID,contactJid.full());
				addressAction->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(streamJid,contactJid) : QIcon());
				connect(addressAction,SIGNAL(triggered()),SLOT(onChangeWindowAddressAction()));
				AMenu->addAction(addressAction,streamGroup);
			}
		}
	}
}

void ChatMessageHandler::onWindowContextMenuRequested(Menu *AMenu)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	if (widget && FRostersModel && FRostersView)
	{
		IRosterIndex *index = FRostersModel->findContactIndexes(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()).value(0);
		if (index)
			FRostersView->contextMenuForIndex(QList<IRosterIndex *>()<<index,NULL,AMenu);
	}
}

void ChatMessageHandler::onWindowToolTipsRequested(QMap<int,QString> &AToolTips)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	if (widget && FRostersModel && FRostersView)
	{
		IRosterIndex *index = FRostersModel->findContactIndexes(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()).value(0);
		if (index)
			FRostersView->toolTipsForIndex(index,NULL,AToolTips);
	}
}

void ChatMessageHandler::onWindowNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	IMessageTabPageNotifier *notifier = qobject_cast<IMessageTabPageNotifier *>(sender());
	IMessageChatWindow *window = notifier!=NULL ? qobject_cast<IMessageChatWindow *>(notifier->tabPage()->instance()) : NULL;
	if (window)
		updateWindow(window);
}

void ChatMessageHandler::onWindowContentAppended(const QString &AHtml, const IMessageContentOptions &AOptions)
{
	IMessageViewWidget *viewWidget = qobject_cast<IMessageViewWidget *>(sender());
	IMessageChatWindow *window = viewWidget!=NULL ? qobject_cast<IMessageChatWindow *>(viewWidget->messageWindow()->instance()) : NULL;
	if (window && FHistoryRequests.values().contains(window))
	{
		WindowContent content;
		content.html = AHtml;
		content.options = AOptions;
		FPendingContent[window].append(content);
		LOG_STRM_DEBUG(window->streamJid(),QString("Added pending content to chat window, with=%1").arg(window->contactJid().bare()));
	}
}

void ChatMessageHandler::onWindowMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared)
{
	Q_UNUSED(AOptions);
	IMessageViewWidget *viewWidget = qobject_cast<IMessageViewWidget *>(sender());
	IMessageChatWindow *window = viewWidget!=NULL ? qobject_cast<IMessageChatWindow *>(viewWidget->messageWindow()->instance()) : NULL;
	if (window)
	{
		if (ACleared)
			FWindowStatus[window].lastDateSeparator = QDate();
		LOG_STRM_DEBUG(window->streamJid(),QString("Chat window style options changed, with=%1, cleared=%2").arg(window->contactJid().bare()).arg(ACleared));
	}
}

void ChatMessageHandler::onStatusIconsChanged()
{
	foreach(IMessageChatWindow *window, FWindows)
		updateWindow(window);
}

void ChatMessageHandler::onAvatarChanged(const Jid &AContactJid)
{
	foreach(IMessageChatWindow *window, FWindows)
		if (window->contactJid() && AContactJid)
			updateWindow(window);
}

void ChatMessageHandler::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (AItem.name!=ABefore.name || AItem.subscription!=ABefore.subscription)
	{
		IMessageChatWindow *window = findWindow(ARoster->streamJid(),AItem.itemJid);
		if (window)
			updateWindow(window);
	}
}

void ChatMessageHandler::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.show!=ABefore.show || AItem.status!=ABefore.status)
	{
		IMessageChatWindow *window = findWindow(APresence->streamJid(),AItem.itemJid);
		if (window)
		{
			if (Options::node(OPV_MESSAGES_SHOWSTATUS).value().toBool())
			{
				QString show = FStatusChanger ? FStatusChanger->nameByShow(AItem.show) : QString::null;
				QString name = FMessageStyles!=NULL ? FMessageStyles->contactName(APresence->streamJid(),AItem.itemJid) : AItem.itemJid.uBare();
				if (!AItem.itemJid.resource().isEmpty() && name!=AItem.itemJid.resource())
					name += "/" + AItem.itemJid.resource();
				QString message = tr("%1 changed status to [%2] %3").arg(name,show,AItem.status);
				showStyledStatus(window,message);
			}
			updateWindow(window);
		}
	}
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
	IMessageChatWindow *window = action!=NULL ? qobject_cast<IMessageChatWindow *>(action->parent()) : NULL;
	if (window)
		window->viewWidget()->clearContent();
}

void ChatMessageHandler::onChangeWindowAddressAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		IMessageChatWindow *window = findWindow(streamJid,contactJid);
		if (window)
		{
			LOG_STRM_DEBUG(window->streamJid(),QString("Changing chat window address by action, with=%1").arg(window->contactJid().bare()));
			window->address()->setAddress(streamJid,contactJid);
		}
	}
}

void ChatMessageHandler::onActiveStreamRemoved(const Jid &AStreamJid)
{
	foreach(IMessageChatWindow *window, FWindows)
		window->address()->removeAddress(AStreamJid);
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

void ChatMessageHandler::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IMessageChatWindow *window = FHistoryRequests.take(AId);
		LOG_STRM_WARNING(window->streamJid(),QString("Failed to load chat history, with=%1, id=%2: %3").arg(window->contactJid().bare(),AId,AError.condition()));
		showStyledStatus(window,tr("Failed to load history: %1").arg(AError.errorMessage()),true);
		FPendingMessages.remove(window);
		FPendingContent.remove(window);
	}
}

void ChatMessageHandler::onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody)
{
	if (FHistoryRequests.contains(AId))
	{
		IMessageChatWindow *window = FHistoryRequests.take(AId);
		window->viewWidget()->clearContent();

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

		foreach(const WindowContent &content, FPendingContent.take(window))
		{
			showDateSeparator(window,content.options.time);
			window->viewWidget()->appendHtml(content.html,content.options);
		}

		WindowStatus &wstatus = FWindowStatus[window];
		wstatus.startTime = !ABody.messages.isEmpty() ? ABody.messages.last().dateTime() : QDateTime();

		LOG_STRM_INFO(window->streamJid(),QString("Chat history loaded and shown, with=%1, id=%2").arg(window->contactJid().bare(),AId));
	}
}

void ChatMessageHandler::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes) && AIndexes.count()==1)
	{
		Action *action = new Action(AMenu);
		action->setText(tr("Open chat dialog"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHATMHANDLER_MESSAGE);
		action->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID));
		action->setData(ADR_CONTACT_JID,AIndexes.first()->data(RDR_FULL_JID));
		action->setShortcutId(SCT_ROSTERVIEW_SHOWCHATDIALOG);
		AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER,true);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
	}
}

void ChatMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IMessageChatWindow *window, FWindows)
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
