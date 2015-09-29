#include "multiuserchatwindow.h"

#include <QPair>
#include <QTimer>
#include <QInputDialog>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <definitions/namespaces.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/toolbargroups.h>
#include <definitions/actiongroups.h>
#include <definitions/dataformtypes.h>
#include <definitions/recentitemtypes.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/multiuseritemkinds.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/multiusertooltiporders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/messagewindowwidgets.h>
#include <definitions/messageviewurlhandlerorders.h>
#include <definitions/messageeditsendhandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/pluginhelper.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/logger.h>

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

#define DEFAULT_USERS_LIST_WIDTH    130

#define MUC_URL_SCHEME              "muc"
#define MUC_URL_GRANTVOICE          "GrantVoice"
#define MUC_URL_REQUESTVOICE        "RequestVoice"

MultiUserChatWindow::MultiUserChatWindow(IMultiUserChatManager *AMultiChatManager, IMultiUserChat *AMultiChat) : QMainWindow(NULL)
{
	REPORT_VIEW;
	setContentsMargins(3,3,3,3);
	setAttribute(Qt::WA_DeleteOnClose, false);

	FUsersView = NULL;
	FInfoWidget = NULL;
	FViewWidget = NULL;
	FEditWidget = NULL;
	FMenuBarWidget = NULL;
	FToolBarWidget = NULL;
	FStatusBarWidget = NULL;
	FTabPageNotifier = NULL;

	FSHIAnyStanza = -1;

	FStateLoaded = false;
	FShownDetached = false;
	FInitializeConfig = false;
	FDestroyOnChatClosed = false;

	FStartCompletePos = 0;
	FCompleteIt = FCompleteNicks.constEnd();

	FMultiChatManager = AMultiChatManager;

	FMultiChat = AMultiChat;
	FMultiChat->instance()->setParent(this);
	connect(FMultiChat->instance(),SIGNAL(stateChanged(int)),SLOT(onMultiChatStateChanged(int)));
	connect(FMultiChat->instance(),SIGNAL(roomTitleChanged(const QString &)),SLOT(onMultiChatRoomTitleChanged(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onMultiChatRequestFailed(const QString &, const XmppError &)));
	connect(FMultiChat->instance(),SIGNAL(presenceChanged(const IPresenceItem &)),SLOT(onMultiChatPresenceChanged(const IPresenceItem &)));
	connect(FMultiChat->instance(),SIGNAL(invitationDeclined(const Jid &, const QString &)),SLOT(onMultiChatInvitationDeclined(const Jid &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userChanged(IMultiUser *, int, const QVariant &)),SLOT(onMultiChatUserChanged(IMultiUser *, int, const QVariant &)));
	connect(FMultiChat->instance(),SIGNAL(voiceRequestReceived(const Message &)),SLOT(onMultiChatVoiceRequestReceived(const Message &)));
	connect(FMultiChat->instance(),SIGNAL(subjectChanged(const QString &, const QString &)),SLOT(onMultiChatSubjectChanged(const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userKicked(const QString &, const QString &, const QString &)),SLOT(onMultiChatUserKicked(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userBanned(const QString &, const QString &, const QString &)),SLOT(onMultiChatUserBanned(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(roomConfigLoaded(const QString &, const IDataForm &)), SLOT(onMultiChatRoomConfigLoaded(const QString &, const IDataForm &)));
	connect(FMultiChat->instance(),SIGNAL(roomConfigUpdated(const QString &, const IDataForm &)), SLOT(onMultiChatRoomConfigUpdated(const QString &, const IDataForm &)));
	connect(FMultiChat->instance(),SIGNAL(roomDestroyed(const QString &, const QString &)), SLOT(onMultiChatRoomDestroyed(const QString &, const QString &)));

	FMainSplitter = new SplitterWidget(this,Qt::Vertical);
	FMainSplitter->setSpacing(3);
	setCentralWidget(FMainSplitter);

	FCentralSplitter = new SplitterWidget(FMainSplitter,Qt::Horizontal);
	FCentralSplitter->setSpacing(3);
	connect(FCentralSplitter,SIGNAL(handleMoved(int,int)),SLOT(onCentralSplitterHandleMoved(int,int)));

	FViewSplitter = new SplitterWidget(FCentralSplitter,Qt::Vertical);
	FViewSplitter->setSpacing(3);

	FUsersSplitter = new SplitterWidget(FCentralSplitter,Qt::Vertical);
	FUsersSplitter->setSpacing(3);

	FMainSplitter->insertWidget(MUCWW_CENTRALSPLITTER,FCentralSplitter,100);
	FCentralSplitter->insertWidget(MUCWW_VIEWSPLITTER,FViewSplitter,75);
	FCentralSplitter->insertWidget(MUCWW_USERSSPLITTER,FUsersSplitter,25,MUCWW_USERSHANDLE);
	FCentralSplitter->setHandleCollapsible(MUCWW_USERSHANDLE,true);
	FCentralSplitter->setHandleStretchable(MUCWW_USERSHANDLE,false);

	connect(this,SIGNAL(tabPageActivated()),SLOT(onMultiChatWindowActivated()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	initialize();
	createMessageWidgets();
	createStaticRoomActions();
	setMultiChatMessageStyle();

	FMultiChat->setAutoPresence(true);
	updateMultiChatWindow();
	
	if (FMultiChat->isOpen())
		onMultiChatStateChanged(FMultiChat->state());
}

MultiUserChatWindow::~MultiUserChatWindow()
{
	FMultiChat->abortConnection(QString::null,false);

	QList<IMessageChatWindow *> chatWindows = FPrivateChatWindows;
	foreach(IMessageChatWindow *window,chatWindows)
		delete window->instance();

	if (FMessageProcessor)
	{
		FMessageProcessor->removeMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
	}

	if (FMessageWidgets)
	{
		FMessageWidgets->removeViewUrlHandler(MVUHO_MULTIUSERCHAT,this);
		FMessageWidgets->removeEditSendHandler(MESHO_MULTIUSERCHATWINDOW_COMMANDS,this);
		FMessageWidgets->removeEditSendHandler(MESHO_MULTIUSERCHATWINDOW_GROUPCHAT,this);
		FMessageWidgets->removeEditSendHandler(MESHO_MULTIUSERCHATWINDOW_PRIVATECHAT,this);
	}

	emit tabPageDestroyed();
}

Jid MultiUserChatWindow::streamJid() const
{
	return FMultiChat->streamJid();
}

Jid MultiUserChatWindow::contactJid() const
{
	return FMultiChat->roomJid();
}

IMessageAddress *MultiUserChatWindow::address() const
{
	return FAddress;
}

IMessageInfoWidget *MultiUserChatWindow::infoWidget() const
{
	return FInfoWidget;
}

IMessageViewWidget *MultiUserChatWindow::viewWidget() const
{
	return FViewWidget;
}

IMessageEditWidget *MultiUserChatWindow::editWidget() const
{
	return FEditWidget;
}

IMessageMenuBarWidget *MultiUserChatWindow::menuBarWidget() const
{
	return FMenuBarWidget;
}

IMessageToolBarWidget *MultiUserChatWindow::toolBarWidget() const
{
	return FToolBarWidget;
}

IMessageStatusBarWidget *MultiUserChatWindow::statusBarWidget() const
{
	return FStatusBarWidget;
}

IMessageReceiversWidget *MultiUserChatWindow::receiversWidget() const
{
	return NULL;
}

SplitterWidget *MultiUserChatWindow::messageWidgetsBox() const
{
	return FMainSplitter;
}

QString MultiUserChatWindow::tabPageId() const
{
	return "MultiUserChatWindow|"+streamJid().pBare()+"|"+contactJid().pBare();
}

bool MultiUserChatWindow::isVisibleTabPage() const
{
	return window()->isVisible();
}

bool MultiUserChatWindow::isActiveTabPage() const
{
	return isVisible() && WidgetManager::isActiveWindow(this);
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
	return QString::null;
}

IMessageTabPageNotifier *MultiUserChatWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void MultiUserChatWindow::setTabPageNotifier(IMessageTabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

bool MultiUserChatWindow::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	Q_UNUSED(AAccept); Q_UNUSED(AStreamJid);
	Jid fromJid = AStanza.from();
	if (AHandlerId==FSHIAnyStanza && contactJid().pBare()==fromJid.pBare())
	{
		if (AStanza.tagName() == "message")
			FLastStanzaTime = QDateTime::currentDateTime().addSecs(1);
		else
			FLastStanzaTime = QDateTime::currentDateTime();
	}
	return false;
}

bool MultiUserChatWindow::messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget)
{
	Q_UNUSED(AOrder); Q_UNUSED(AWidget);
	return false;
}

bool MultiUserChatWindow::messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget)
{
	if (AOrder == MESHO_MULTIUSERCHATWINDOW_COMMANDS)
	{
		if (AWidget == FEditWidget)
			return execShortcutCommand(AWidget->textEdit()->toPlainText());
	}
	else if (AOrder == MESHO_MULTIUSERCHATWINDOW_GROUPCHAT)
	{
		if (AWidget==FEditWidget && FMultiChat->isOpen())
		{
			Message message;
			if (FMessageProcessor)
				FMessageProcessor->textToMessage(message,AWidget->document());
			else
				message.setBody(AWidget->document()->toPlainText());
			return !message.body().isEmpty() && FMultiChat->sendMessage(message);
		}
	}
	else if (AOrder == MESHO_MULTIUSERCHATWINDOW_PRIVATECHAT)
	{
		IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(AWidget->messageWindow()->instance());
		if (FPrivateChatWindows.contains(window) && FMultiChat->isOpen() && FMultiChat->findUser(window->contactJid().resource())!=NULL)
		{
			Message message;
			if (FMessageProcessor)
				FMessageProcessor->textToMessage(message,AWidget->document());
			else
				message.setBody(AWidget->document()->toPlainText());
			return !message.body().isEmpty() && FMultiChat->sendMessage(message,window->contactJid().resource());
		}
	}
	return false;
}

bool MultiUserChatWindow::messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl)
{
	if (AOrder==MVUHO_MULTIUSERCHAT && AWidget==FViewWidget && AUrl.isValid() && AUrl.scheme()==MUC_URL_SCHEME)
	{
		QString action = AUrl.fragment();
		if (action==MUC_URL_GRANTVOICE && FMultiChat->isOpen())
		{
			const struct { QString var; QString value; } fileds[] =
			{
				{ "FORM_TYPE",         QString(DFT_MUC_REQUEST)  },
				{ "muc#role",          AUrl.queryItemValue("role")     },
				{ "muc#jid",           AUrl.queryItemValue("jid")      },
				{ "muc#roomnick",      AUrl.queryItemValue("roomnick") },
				{ "muc#request_allow", QString("true")                 },
				{ QString::null,       QString::null                   }
			};

			IDataForm form;
			form.type = DATAFORM_TYPE_SUBMIT;
			for (int i=0; !fileds[i].var.isNull(); i++)
			{
				IDataField field;
				field.var = fileds[i].var;
				field.value = fileds[i].value;
				form.fields.append(field);
			}

			Message message;
			message.setTo(contactJid().bare()).setId(AUrl.queryItemValue("id"));
			
			QDomElement formElem = message.stanza().element();
			FDataForms->xmlForm(form,formElem);
			
			QString nick = AUrl.queryItemValue("roomnick");
			IMultiUser *user = FMultiChat->findUser(nick);

			if (user == NULL)
				showMultiChatStatusMessage(tr("User %1 was not found in the conference").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else if (user->role() != MUC_ROLE_VISITOR)
				showMultiChatStatusMessage(tr("User %1 already has a voice in the conference").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else if (!FMultiChat->sendVoiceApproval(message))
				showMultiChatStatusMessage(tr("Unable to grant a voice to the user %1").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else
				showMultiChatStatusMessage(tr("You granted the voice to the user %1").arg(nick),IMessageStyleContentOptions::TypeNotification);
		}
		else if (action==MUC_URL_REQUESTVOICE && FMultiChat->isOpen())
		{
			FRequestVoice->trigger();
		}
		else
		{
			REPORT_ERROR(QString("Unexpected internal conference URL action: %1").arg(action));
		}
		return true;
	}
	return false;
}

bool MultiUserChatWindow::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder);
	if (ADirection == IMessageProcessor::DirectionIn)
		return streamJid()==AMessage.to() && contactJid().pBare()==Jid(AMessage.from()).pBare();
	else
		return streamJid()==AMessage.from() && contactJid().pBare()==Jid(AMessage.to()).pBare();
}

bool MultiUserChatWindow::messageDisplay(const Message &AMessage, int ADirection)
{
	bool displayed = false;
	if (AMessage.type() == Message::Error)
	{
		displayed = true;
		Jid userJid = AMessage.from();
		XmppStanzaError err(AMessage.stanza());
		QString text = !userJid.resource().isEmpty() ? QString("%1: %2").arg(userJid.resource()).arg(err.errorMessage()) : err.errorMessage();
		showMultiChatStatusMessage(text,IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
	}
	else if (ADirection == IMessageProcessor::DirectionIn)
	{
		Jid userJid = AMessage.from();
		bool isEmptyMessage = AMessage.body().isEmpty();
		bool isServiceMessage = userJid.resource().isEmpty();

		// Voice Requests
		if (!AMessage.stanza().firstElement("x",DFT_MUC_REQUEST).isNull())
		{
			QDomElement requestElem = AMessage.stanza().firstElement("x",DFT_MUC_REQUEST);

			Jid reqJid = requestElem.firstChildElement("jid").text();
			QString reqRole = requestElem.firstChildElement("role").text();
			QString reqNick = requestElem.firstChildElement("roomnick").text();

			IMultiUser *user = FMultiChat->findUser(reqNick);
			if (user!=NULL && user->role()==MUC_ROLE_VISITOR)
			{
				displayed = true;

				QUrl url;
				url.setScheme(MUC_URL_SCHEME);
				url.setPath(user->userJid().full());
				url.setFragment(MUC_URL_GRANTVOICE);
				url.addQueryItem("id",AMessage.id());
				url.addQueryItem("jid",reqJid.full());
				url.addQueryItem("role",reqRole);
				url.addQueryItem("roomnick",reqNick);

				QString html = tr("User %1 requests a voice in the conference, %2").arg(Qt::escape(reqNick),QString("<a href='%1'>%2</a>").arg(url.toString(),tr("Grant Voice")));
				showHTMLStatusMessage(FViewWidget,html,IMessageStyleContentOptions::TypeNotification);
			}
		}
		// Service messages
		else if (isServiceMessage)
		{
			if (!showMultiChatStatusCodes(FMultiChat->statusCodes(AMessage.stanza()),QString::null,AMessage.body()) && !isEmptyMessage)
				showMultiChatStatusMessage(AMessage.body(),IMessageStyleContentOptions::TypeNotification);
		}
		// Private messages
		else if (AMessage.type()!=Message::GroupChat && !isEmptyMessage)
		{
			IMessageChatWindow *window = getPrivateChatWindow(userJid);
			if (window)
			{
				displayed = true;
				if (!AMessage.isDelayed())
					updateRecentItemActiveTime(window);
				if (FHistoryRequests.values().contains(window))
					FPendingMessages[window].append(AMessage);
				showPrivateChatMessage(window,AMessage);
			}
			else
			{
				LOG_STRM_ERROR(streamJid(),QString("Failed to show incoming private chat message, room=%1, user=%2: Private chat window is not created").arg(contactJid().bare(),userJid.resource()));
			}
		}
		// Groupchat messages
		else if (AMessage.type()==Message::GroupChat && !isEmptyMessage)
		{
			displayed = true;
			if (!AMessage.isDelayed())
				updateRecentItemActiveTime(NULL);
			if (FHistoryRequests.values().contains(NULL))
				FPendingMessages[NULL].append(AMessage);
			showMultiChatUserMessage(AMessage,userJid.resource());
		}
	}
	else if (ADirection == IMessageProcessor::DirectionOut)
	{
		Jid userJid = AMessage.to();

		// Private chat messages
		if (AMessage.type()!=Message::GroupChat && !AMessage.body().isEmpty())
		{
			IMessageChatWindow *window = getPrivateChatWindow(userJid);
			if (window)
			{
				displayed = true;
				if (!AMessage.isDelayed())
					updateRecentItemActiveTime(window);
				if (FHistoryRequests.values().contains(window))
					FPendingMessages[window].append(AMessage);
				showPrivateChatMessage(window,AMessage);
			}
			else
			{
				LOG_STRM_ERROR(streamJid(),QString("Failed to show outgoing private chat message, nick=%1, room=%2: Private chat window is not created").arg(userJid.resource(),contactJid().bare()));
			}
		}
	}
	return displayed;
}

INotification MultiUserChatWindow::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection==IMessageProcessor::DirectionIn && AMessage.type()!=Message::Error)
	{
		Jid userJid = AMessage.from();
		bool isEmptyMessage = AMessage.body().isEmpty();
		bool isServiceMessage = userJid.resource().isEmpty();
		int messageId = AMessage.data(MDR_MESSAGE_ID).toInt();

		IMessageTabPage *page = NULL;
		IconStorage *iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

		// Voice Requests
		if (!AMessage.stanza().firstElement("x",DFT_MUC_REQUEST).isNull())
		{
			IMultiUser *user = FMultiChat->findUser(AMessage.stanza().firstElement("x",DFT_MUC_REQUEST).firstChildElement("roomnick").text());
			if (user)
			{
				page = this;
				userJid = user->userJid();

				notify.typeId = NNT_MUC_MESSAGE_GROUPCHAT;
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_GROUPCHAT);

				notify.data.insert(NDR_STREAM_JID,streamJid().full());
				notify.data.insert(NDR_CONTACT_JID,userJid.bare());
				notify.data.insert(NDR_ICON,iconStorage->getIcon(MNI_MUC_MESSAGE));
				notify.data.insert(NDR_TOOLTIP,tr("Voice request from [%1]").arg(userJid.resource()));

				notify.data.insert(NDR_ROSTER_ORDER,RNO_GROUPCHATMESSAGE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);

				notify.data.insert(NDR_POPUP_CAPTION,tr("Voice request received"));
				notify.data.insert(NDR_POPUP_TITLE,tr("[%1] in [%2]").arg(userJid.resource()).arg(userJid.uNode()));
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(userJid));
				notify.data.insert(NDR_POPUP_TEXT,tr("User %1 requests a voice in the conference").arg(userJid.resource()));

				notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);

				if (!isActiveTabPage())
					FActiveMessages.append(messageId);
				else
					notify.kinds &= Options::node(OPV_NOTIFICATIONS_FORCESOUND).value().toBool() ? INotification::SoundPlay : 0;
			}
		}
		// Private messages
		else if (AMessage.type()!=Message::GroupChat && !isServiceMessage && !isEmptyMessage)
		{
			IMessageChatWindow *window = findPrivateChatWindow(userJid);
			if (window)
			{
				page = window;

				notify.typeId = NNT_MUC_MESSAGE_PRIVATE;
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_PRIVATE);

				notify.data.insert(NDR_STREAM_JID,streamJid().full());
				notify.data.insert(NDR_CONTACT_JID,userJid.full());
				notify.data.insert(NDR_ICON,iconStorage->getIcon(MNI_MUC_PRIVATE_MESSAGE));
				notify.data.insert(NDR_TOOLTIP,tr("Private message from [%1]").arg(userJid.resource()));

				notify.data.insert(NDR_ROSTER_ORDER,RNO_GROUPCHATMESSAGE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
				
				notify.data.insert(NDR_POPUP_CAPTION,tr("Private message"));
				notify.data.insert(NDR_POPUP_TITLE,QString("[%1]").arg(userJid.resource()));
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(userJid));
				
				notify.data.insert(NDR_SOUND_FILE,SDF_MUC_PRIVATE_MESSAGE);

				if (!window->isActiveTabPage())
				{
					if (FDestroyTimers.contains(window))
						delete FDestroyTimers.take(window);
					FActiveChatMessages.insertMulti(window,messageId);

					IMultiUserViewNotify notify;
					notify.flags |= IMultiUserViewNotify::Blink;
					notify.icon = iconStorage->getIcon(MNI_MUC_PRIVATE_MESSAGE);

					QStandardItem *userItem = FUsersView->findUserItem(FMultiChat->findUser(userJid.resource()));
					int notifyId = FUsersView->insertItemNotify(notify,userItem);
					FActiveChatMessageNotify.insert(messageId, notifyId);
				}
				else
				{
					notify.kinds &= Options::node(OPV_NOTIFICATIONS_FORCESOUND).value().toBool() ? INotification::SoundPlay : 0;
				}
			}
		}
		// Groupchat messages
		else if (AMessage.type()==Message::GroupChat && !isServiceMessage && !isEmptyMessage && !AMessage.isDelayed())
		{
			page = this;

			if (isMentionMessage(AMessage))
			{
				notify.typeId = NNT_MUC_MESSAGE_MENTION;
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_MENTION);
				notify.data.insert(NDR_TOOLTIP,tr("Mention message in conference: %1").arg(userJid.uNode()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Mention in conference"));
			}
			else
			{
				notify.typeId = NNT_MUC_MESSAGE_GROUPCHAT;
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_GROUPCHAT);
				notify.data.insert(NDR_TOOLTIP,tr("New message in conference: %1").arg(userJid.uNode()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Conference message"));
			}

			notify.data.insert(NDR_STREAM_JID,streamJid().full());
			notify.data.insert(NDR_CONTACT_JID,userJid.bare());
			notify.data.insert(NDR_ICON,iconStorage->getIcon(MNI_MUC_MESSAGE));

			notify.data.insert(NDR_ROSTER_ORDER,RNO_GROUPCHATMESSAGE);
			notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);

			notify.data.insert(NDR_POPUP_TITLE,tr("[%1] in [%2]").arg(userJid.resource()).arg(userJid.uNode()));
			notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(userJid));

			notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);

			if (!isActiveTabPage())
				FActiveMessages.append(messageId);
			else
				notify.kinds &= Options::node(OPV_NOTIFICATIONS_FORCESOUND).value().toBool() ? INotification::SoundPlay : 0;
		}

		if (notify.kinds & INotification::RosterNotify)
		{
			QMap<QString,QVariant> searchData;
			searchData.insert(QString::number(RDR_STREAM_JID),streamJid().pFull());

			if (FRecentContacts!=NULL && AMessage.type()==Message::Chat)
			{
				searchData.insert(QString::number(RDR_KIND),RIK_RECENT_ITEM);
				searchData.insert(QString::number(RDR_RECENT_TYPE),REIT_CONFERENCE_PRIVATE);
				searchData.insert(QString::number(RDR_RECENT_REFERENCE),userJid.pFull());
			}
			else
			{
				searchData.insert(QString::number(RDR_KIND),RIK_MUC_ITEM);
				searchData.insert(QString::number(RDR_PREP_BARE_JID),userJid.pBare());
			}

			notify.data.insert(NDR_ROSTER_SEARCH_DATA,searchData);
		}

		if ((notify.kinds & INotification::PopupWindow)>0 && !Options::node(OPV_NOTIFICATIONS_HIDEMESSAGE).value().toBool())
		{
			if (FMessageProcessor)
			{
				QTextDocument doc;
				FMessageProcessor->messageToText(&doc,AMessage);
				notify.data.insert(NDR_POPUP_HTML,TextManager::getDocumentBody(doc));
			}
			notify.data.insert(NDR_POPUP_TEXT,AMessage.body());
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
	return notify;
}

bool MultiUserChatWindow::messageShowWindow(int AMessageId)
{
	if (FActiveMessages.contains(AMessageId))
	{
		showTabPage();
		return true;
	}
	else if (FActiveChatMessages.values().contains(AMessageId))
	{
		IMessageChatWindow *window = FActiveChatMessages.key(AMessageId);
		window->showTabPage();
		return true;
	}
	else
	{
		REPORT_ERROR("Failed to show notified conference message window: Window not found");
	}
	return false;
}

bool MultiUserChatWindow::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (streamJid()==AStreamJid && contactJid().pBare()==AContactJid.pBare())
	{
		if (AType == Message::GroupChat)
		{
			if (AShowMode == IMessageHandler::SM_ASSIGN)
				assignTabPage();
			else if (AShowMode == IMessageHandler::SM_SHOW)
				showTabPage();
			else if (AShowMode == IMessageHandler::SM_MINIMIZED)
				showMinimizedTabPage();
			return true;
		}
		else if (AType == Message::Chat)
		{
			IMessageChatWindow *window = getPrivateChatWindow(AContactJid);
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
				LOG_STRM_WARNING(AStreamJid,QString("Failed to show private chat window, nick=%1, room=%2: Private chat window not created").arg(AContactJid.resource(),contactJid().bare()));
			}
		}
	}
	return false;
}

IMultiUserChat *MultiUserChatWindow::multiUserChat() const
{
	return FMultiChat;
}

IMultiUserView *MultiUserChatWindow::multiUserView() const
{
	return FUsersView;
}

SplitterWidget *MultiUserChatWindow::viewWidgetsBox() const
{
	return FViewSplitter;
}

SplitterWidget *MultiUserChatWindow::usersWidgetsBox() const
{
	return FUsersSplitter;
}

SplitterWidget *MultiUserChatWindow::centralWidgetsBox() const
{
	return FCentralSplitter;
}

IMessageChatWindow *MultiUserChatWindow::openPrivateChatWindow(const Jid &AContactJid)
{
	messageShowWindow(MHO_MULTIUSERCHAT_GROUPCHAT,streamJid(),AContactJid,Message::Chat,IMessageHandler::SM_SHOW);
	return findPrivateChatWindow(AContactJid);
}

IMessageChatWindow *MultiUserChatWindow::findPrivateChatWindow(const Jid &AContactJid) const
{
	foreach(IMessageChatWindow *window,FPrivateChatWindows)
		if (window->contactJid() == AContactJid)
			return window;
	return NULL;
}

void MultiUserChatWindow::contextMenuForRoom(Menu *AMenu)
{
	QString role = FMultiChat->isOpen() ? FMultiChat->mainUser()->role() : MUC_ROLE_NONE;
	QString affiliation = FMultiChat->isOpen() ? FMultiChat->mainUser()->affiliation() : MUC_AFFIL_NONE;

	if (affiliation == MUC_AFFIL_OWNER)
	{
		AMenu->addAction(FChangeNick,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FChangeTopic,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FInviteContact,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FEditAffiliations,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FConfigRoom,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FDestroyRoom,AG_RVCM_MULTIUSERCHAT_TOOLS);
	}
	else if (affiliation == MUC_AFFIL_ADMIN)
	{
		AMenu->addAction(FChangeNick,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FChangeTopic,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FInviteContact,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FEditAffiliations,AG_RVCM_MULTIUSERCHAT_TOOLS);
	}
	else if (role == MUC_ROLE_VISITOR)
	{
		AMenu->addAction(FChangeNick,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FRequestVoice,AG_RVCM_MULTIUSERCHAT_COMMON);
	}
	else
	{
		AMenu->addAction(FChangeNick,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FChangeTopic,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FInviteContact,AG_RVCM_MULTIUSERCHAT_COMMON);
	}

	emit multiChatContextMenu(AMenu);
}

void MultiUserChatWindow::contextMenuForUser(IMultiUser *AUser, Menu *AMenu)
{
	if (FUsers.contains(AUser))
	{
		if (FMultiChat->isOpen() && AUser!=FMultiChat->mainUser())
		{
			Action *openChat = new Action(AMenu);
			openChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_PRIVATE_MESSAGE);
			openChat->setText(tr("Open Chat Dialog"));
			openChat->setData(ADR_USER_NICK,AUser->nick());
			connect(openChat,SIGNAL(triggered(bool)),SLOT(onOpenPrivateChatWindowActionTriggered(bool)));
			AMenu->addAction(openChat,AG_MUCM_MULTIUSERCHAT_PRIVATE,true);

			if (FMultiChat->mainUser()->role() == MUC_ROLE_MODERATOR)
			{
				Action *setRoleNone = new Action(AMenu);
				setRoleNone->setText(tr("Kick"));
				setRoleNone->setData(ADR_USER_NICK,AUser->nick());
				setRoleNone->setData(ADR_USER_ROLE,MUC_ROLE_NONE);
				connect(setRoleNone,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
				AMenu->addAction(setRoleNone,AG_MUCM_MULTIUSERCHAT_UTILS,false);

				Action *setAffilOutcast = new Action(AMenu);
				setAffilOutcast->setText(tr("Ban"));
				setAffilOutcast->setData(ADR_USER_NICK,AUser->nick());
				setAffilOutcast->setData(ADR_USER_AFFIL,MUC_AFFIL_OUTCAST);
				connect(setAffilOutcast,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
				AMenu->addAction(setAffilOutcast,AG_MUCM_MULTIUSERCHAT_UTILS,false);

				Menu *changeRole = new Menu(AMenu);
				changeRole->setTitle(tr("Change Role"));
				{
					Action *setRoleVisitor = new Action(changeRole);
					setRoleVisitor->setCheckable(true);
					setRoleVisitor->setText(tr("Visitor"));
					setRoleVisitor->setData(ADR_USER_NICK,AUser->nick());
					setRoleVisitor->setData(ADR_USER_ROLE,MUC_ROLE_VISITOR);
					setRoleVisitor->setChecked(AUser->role() == MUC_ROLE_VISITOR);
					connect(setRoleVisitor,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
					changeRole->addAction(setRoleVisitor,AG_DEFAULT,false);

					Action *setRoleParticipant = new Action(changeRole);
					setRoleParticipant->setCheckable(true);
					setRoleParticipant->setText(tr("Participant"));
					setRoleParticipant->setData(ADR_USER_NICK,AUser->nick());
					setRoleParticipant->setData(ADR_USER_ROLE,MUC_ROLE_PARTICIPANT);
					setRoleParticipant->setChecked(AUser->role() == MUC_ROLE_PARTICIPANT);
					connect(setRoleParticipant,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
					changeRole->addAction(setRoleParticipant,AG_DEFAULT,false);

					Action *setRoleModerator = new Action(changeRole);
					setRoleModerator->setCheckable(true);
					setRoleModerator->setText(tr("Moderator"));
					setRoleModerator->setData(ADR_USER_NICK,AUser->nick());
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
					setAffilNone->setData(ADR_USER_NICK,AUser->nick());
					setAffilNone->setData(ADR_USER_AFFIL,MUC_AFFIL_NONE);
					setAffilNone->setChecked(AUser->affiliation() == MUC_AFFIL_NONE);
					connect(setAffilNone,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
					changeAffiliation->addAction(setAffilNone,AG_DEFAULT,false);

					Action *setAffilMember = new Action(changeAffiliation);
					setAffilMember->setCheckable(true);
					setAffilMember->setText(tr("Member"));
					setAffilMember->setData(ADR_USER_NICK,AUser->nick());
					setAffilMember->setData(ADR_USER_AFFIL,MUC_AFFIL_MEMBER);
					setAffilMember->setChecked(AUser->affiliation() == MUC_AFFIL_MEMBER);
					connect(setAffilMember,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
					changeAffiliation->addAction(setAffilMember,AG_DEFAULT,false);

					Action *setAffilAdmin = new Action(changeAffiliation);
					setAffilAdmin->setCheckable(true);
					setAffilAdmin->setText(tr("Administrator"));
					setAffilAdmin->setData(ADR_USER_NICK,AUser->nick());
					setAffilAdmin->setData(ADR_USER_AFFIL,MUC_AFFIL_ADMIN);
					setAffilAdmin->setChecked(AUser->affiliation() == MUC_AFFIL_ADMIN);
					connect(setAffilAdmin,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
					changeAffiliation->addAction(setAffilAdmin,AG_DEFAULT,false);

					Action *setAffilOwner = new Action(changeAffiliation);
					setAffilOwner->setCheckable(true);
					setAffilOwner->setText(tr("Owner"));
					setAffilOwner->setData(ADR_USER_NICK,AUser->nick());
					setAffilOwner->setData(ADR_USER_AFFIL,MUC_AFFIL_OWNER);
					setAffilOwner->setChecked(AUser->affiliation() == MUC_AFFIL_OWNER);
					connect(setAffilOwner,SIGNAL(triggered(bool)),SLOT(onChangeUserAffiliationActionTriggered(bool)));
					changeAffiliation->addAction(setAffilOwner,AG_DEFAULT,false);
				}
				AMenu->addAction(changeAffiliation->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);
			}
		}
		emit multiUserContextMenu(AUser,AMenu);
	}
}

void MultiUserChatWindow::toolTipsForUser(IMultiUser *AUser, QMap<int,QString> &AToolTips)
{
	if (FUsers.contains(AUser))
	{
		QString avatar = FAvatars!=NULL ? FAvatars->avatarHash(AUser->userJid(),true) : QString::null;
		if (FAvatars->hasAvatar(avatar))
		{
			QString fileName = FAvatars->avatarFileName(avatar);
			QSize imageSize = QImageReader(fileName).size();
			if (imageSize.height()>64 || imageSize.width()>64)
				imageSize.scale(QSize(64,64), Qt::KeepAspectRatio);
			QString avatarMask = "<img src='%1' width=%2 height=%3 />";
			AToolTips.insert(MUTTO_MULTIUSERCHAT_AVATAR,avatarMask.arg(fileName).arg(imageSize.width()).arg(imageSize.height()));
		}

		AToolTips.insert(MUTTO_MULTIUSERCHAT_NICKNAME,QString("<big><b>%1</b></big>").arg(Qt::escape(AUser->nick())));

		if (AUser->realJid().isValid())
			AToolTips.insert(MUTTO_MULTIUSERCHAT_REALJID,tr("<b>Jabber ID:</b> %1").arg(Qt::escape(AUser->realJid().uBare())));

		QString role = AUser->role();
		if (!role.isEmpty())
		{
			QString roleName = role;
			if (role == MUC_ROLE_VISITOR)
				roleName = tr("Visitor");
			else if (role == MUC_ROLE_PARTICIPANT)
				roleName = tr("Participant");
			else if (role == MUC_ROLE_MODERATOR)
				roleName = tr("Moderator");
			AToolTips.insert(MUTTO_MULTIUSERCHAT_ROLE,tr("<b>Role:</b> %1").arg(Qt::escape(roleName)));
		}

		QString affiliation = AUser->affiliation();
		if (!affiliation.isEmpty())
		{
			QString affilName = affiliation;
			if (affiliation == MUC_AFFIL_NONE)
				affilName = tr("None");
			else if (affiliation == MUC_AFFIL_MEMBER)
				affilName = tr("Member");
			else if (affiliation == MUC_AFFIL_ADMIN)
				affilName = tr("Administrator");
			else if (affiliation == MUC_AFFIL_OWNER)
				affilName = tr("Owner");
			AToolTips.insert(MUTTO_MULTIUSERCHAT_AFFILIATION,tr("<b>Affiliation:</b> %1").arg(Qt::escape(affilName)));
		}

		QString ttStatus;
		QString statusText = AUser->presence().status;
		QString statusName = FStatusChanger!=NULL ? FStatusChanger->nameByShow(AUser->presence().show) : QString::null;
		ttStatus = tr("<b>Status:</b> %1").arg(Qt::escape(statusName));
		if (!statusText.isEmpty())
			ttStatus +="<br>"+Qt::escape(statusText).replace('\n',"<br>");
		AToolTips.insert(MUTTO_MULTIUSERCHAT_STATUS,ttStatus);

		emit multiUserToolTips(AUser,AToolTips);
	}
}

void MultiUserChatWindow::exitAndDestroy(const QString &AStatus, int AWaitClose)
{
	closeTabPage();
	FDestroyOnChatClosed = true;

	if (FMultiChat->state() != IMultiUserChat::Closed)
	{
		FMultiChat->sendPresence(IPresence::Offline,AStatus,0);
		showMultiChatStatusMessage(tr("Leaving conference..."),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusOffline);
		QTimer::singleShot(AWaitClose, this, SLOT(deleteLater()));
	}
	else
	{
		deleteLater();
	}
}

void MultiUserChatWindow::initialize()
{
	FMessageWidgets = PluginHelper::pluginInstance<IMessageWidgets>();
	if (FMessageWidgets)
	{
		FMessageWidgets->insertViewUrlHandler(MVUHO_MULTIUSERCHAT,this);
		FMessageWidgets->insertEditSendHandler(MESHO_MULTIUSERCHATWINDOW_COMMANDS,this);
		FMessageWidgets->insertEditSendHandler(MESHO_MULTIUSERCHATWINDOW_GROUPCHAT,this);
		FMessageWidgets->insertEditSendHandler(MESHO_MULTIUSERCHATWINDOW_PRIVATECHAT,this);
	}

	FStatusIcons = PluginHelper::pluginInstance<IStatusIcons>();
	if (FStatusIcons)
	{
		connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
	}

	FMessageProcessor = PluginHelper::pluginInstance<IMessageProcessor>();
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
	}

	FMessageStyleManager = PluginHelper::pluginInstance<IMessageStyleManager>();
	if (FMessageStyleManager)
	{
		connect(FMessageStyleManager->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
			SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
	}

	FMessageArchiver = PluginHelper::pluginInstance<IMessageArchiver>();
	if (FMessageArchiver)
	{
		connect(FMessageArchiver->instance(),SIGNAL(messagesLoaded(const QString &, const IArchiveCollectionBody &)),
			SLOT(onArchiveMessagesLoaded(const QString &, const IArchiveCollectionBody &)));
		connect(FMessageArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
			SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));
	}

	FAvatars = PluginHelper::pluginInstance<IAvatars>();
	FDataForms = PluginHelper::pluginInstance<IDataForms>();
	FStatusChanger = PluginHelper::pluginInstance<IStatusChanger>();
	FRecentContacts = PluginHelper::pluginInstance<IRecentContacts>();
	FStanzaProcessor = PluginHelper::pluginInstance<IStanzaProcessor>();
}

void MultiUserChatWindow::createMessageWidgets()
{
	if (FMessageWidgets)
	{
		FAddress = FMessageWidgets->newAddress(FMultiChat->streamJid(),FMultiChat->roomJid(),this);

		FInfoWidget = FMessageWidgets->newInfoWidget(this,FMainSplitter);
		FMainSplitter->insertWidget(MUCWW_INFOWIDGET,FInfoWidget->instance());

		FViewWidget = FMessageWidgets->newViewWidget(this,FViewSplitter);
		connect(FViewWidget->instance(),SIGNAL(contentAppended(const QString &, const IMessageStyleContentOptions &)),
			SLOT(onMultiChatContentAppended(const QString &, const IMessageStyleContentOptions &)));
		connect(FViewWidget->instance(),SIGNAL(messageStyleOptionsChanged(const IMessageStyleOptions &, bool)),
			SLOT(onMultiChatMessageStyleOptionsChanged(const IMessageStyleOptions &, bool)));
		FViewSplitter->insertWidget(MUCWW_VIEWWIDGET,FViewWidget->instance(),100);
		FWindowStatus[FViewWidget].createTime = QDateTime::currentDateTime();

		FUsersView = new MultiUserView(FMultiChat,FUsersSplitter);
		FUsersView->instance()->viewport()->installEventFilter(this);
		FUsersView->setViewMode(Options::node(OPV_MUC_GROUPCHAT_USERVIEWMODE).value().toInt());
		connect(FUsersView->instance(),SIGNAL(itemNotifyActivated(int)),SLOT(onMultiChatUserItemNotifyActivated(int)));
		connect(FUsersView->instance(),SIGNAL(doubleClicked(const QModelIndex &)),SLOT(onMultiChatUserItemDoubleClicked(const QModelIndex &)));
		connect(FUsersView->instance(),SIGNAL(itemContextMenu(QStandardItem *, Menu *)),SLOT(onMultiChatUserItemContextMenu(QStandardItem *, Menu *)));
		connect(FUsersView->instance(),SIGNAL(itemToolTips(QStandardItem *, QMap<int,QString> &)),SLOT(onMultiChatUserItemToolTips(QStandardItem *, QMap<int,QString> &)));
		FUsersSplitter->insertWidget(MUCWW_USERSWIDGET,FUsersView->instance(),100);

		FEditWidget = FMessageWidgets->newEditWidget(this,FMainSplitter);
		FEditWidget->setSendShortcutId(SCT_MESSAGEWINDOWS_SENDCHATMESSAGE);
		connect(FEditWidget->instance(),SIGNAL(keyEventReceived(QKeyEvent *,bool &)),SLOT(onMultiChatEditWidgetKeyEvent(QKeyEvent *,bool &)));
		FMainSplitter->insertWidget(MUCWW_EDITWIDGET,FEditWidget->instance());

		FToolBarWidget = FMessageWidgets->newToolBarWidget(this,FMainSplitter);
		FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
		FMainSplitter->insertWidget(MUCWW_TOOLBARWIDGET,FToolBarWidget->instance());

		FMenuBarWidget = FMessageWidgets->newMenuBarWidget(this,this);
		setMenuBar(FMenuBarWidget->instance());

		FStatusBarWidget = FMessageWidgets->newStatusBarWidget(this,this);
		setStatusBar(FStatusBarWidget->instance());

		setTabPageNotifier(FMessageWidgets->newTabPageNotifier(this));
		connect(tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onMultiChatNotifierActiveNotifyChanged(int)));
	}
}

void MultiUserChatWindow::createStaticRoomActions()
{
	FChangeNick = new Action(this);
	FChangeNick->setText(tr("Change Nick"));
	FChangeNick->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_NICK);
	connect(FChangeNick,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FChangeTopic = new Action(this);
	FChangeTopic->setText(tr("Change Topic"));
	FChangeTopic->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_TOPIC);
	connect(FChangeTopic,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FInviteContact = new Action(this);
	FInviteContact->setText(tr("Invite to Conference"));
	FInviteContact->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
	connect(FInviteContact,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FRequestVoice = new Action(this);
	FRequestVoice->setText(tr("Request Voice"));
	FRequestVoice->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_REQUEST_VOICE);
	connect(FRequestVoice,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FEditAffiliations = new Action(this);
	FEditAffiliations->setText(tr("Edit Users Affiliations"));
	FEditAffiliations->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_AFFILIATIONS);
	connect(FEditAffiliations,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FConfigRoom = new Action(this);
	FConfigRoom->setText(tr("Configure Conference"));
	FConfigRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFIGURE_ROOM);
	connect(FConfigRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FDestroyRoom = new Action(this);
	FDestroyRoom->setText(tr("Destroy Conference"));
	FDestroyRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_DESTROY_ROOM);
	connect(FDestroyRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FClearChat = new Action(this);
	FClearChat->setToolTip(tr("Clear window"));
	FClearChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
	connect(FClearChat,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));
	FToolBarWidget->toolBarChanger()->insertAction(FClearChat,TBG_MWTBW_CLEAR_WINDOW);

	FEnterRoom = new Action(this);
	FEnterRoom->setText(tr("Enter"));
	FEnterRoom->setToolTip(tr("Enter conference"));
	FEnterRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
	connect(FEnterRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));
	QToolButton *enterButton = FToolBarWidget->toolBarChanger()->insertAction(FEnterRoom, TBG_MCWTBW_ROOM_ENTER);
	enterButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	FExitRoom = new Action(this);
	FExitRoom->setText(tr("Exit"));
	FExitRoom->setToolTip(tr("Exit conference"));
	FExitRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
	connect(FExitRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));
	QToolButton *exitButton = FToolBarWidget->toolBarChanger()->insertAction(FExitRoom, TBG_MCWTBW_ROOM_EXIT);
	exitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	FUsersHide = new Action(this);
	FUsersHide->setCheckable(true);
	FUsersHide->setToolTip(tr("Hide/Show Participants List"));
	FUsersHide->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_USERS_HIDE);
	connect(FUsersHide,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));
	FToolBarWidget->toolBarChanger()->insertAction(FUsersHide, TBG_MCWTBW_USERS_HIDE);
}

void MultiUserChatWindow::saveWindowState()
{
	if (FStateLoaded)
	{
		int width = FCentralSplitter->handleSize(MUCWW_USERSHANDLE);
		if (width > 0)
		{
			Options::setFileValue(width,"muc.mucwindow.users-list-width",tabPageId());
			Options::setFileValue(false,"muc.mucwindow.users-list-hidden",tabPageId());
		}
		else
		{
			Options::setFileValue(true,"muc.mucwindow.users-list-hidden",tabPageId());
		}
	}
}

void MultiUserChatWindow::loadWindowState()
{
	int size = Options::fileValue("muc.mucwindow.users-list-width",tabPageId()).toInt();
	bool hidden = Options::fileValue("muc.mucwindow.users-list-hidden",tabPageId()).toBool();

	if (hidden)
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,0);
	else if (size > 0)
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,size);
	else
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,DEFAULT_USERS_LIST_WIDTH);
	FUsersHide->setChecked(!hidden);
	
	FStateLoaded = true;
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

void MultiUserChatWindow::refreshCompleteNicks()
{
	QMultiMap<QString,QString> sortedNicks;
	foreach(IMultiUser *user, FUsers.keys())
	{
		if (user != FMultiChat->mainUser())
			if (FCompleteNickStarts.isEmpty() || user->nick().toLower().startsWith(FCompleteNickStarts))
				sortedNicks.insertMulti(user->nick().toLower(), user->nick());
	}
	FCompleteNicks = sortedNicks.values();

	int curNickIndex = FCompleteNicks.indexOf(FCompleteNickLast);
	FCompleteIt = FCompleteNicks.constBegin() + (curNickIndex >= 0 ? curNickIndex : 0);
}

bool MultiUserChatWindow::execShortcutCommand(const QString &AText)
{
	bool hasCommand = false;

	if (AText.startsWith("/kick ") || AText=="/kick")
	{
		QStringList params = AText.split(" ");
		QString nick = params.value(1);
		if (!nick.isEmpty())
		{
			QString reason = QStringList(params.mid(2)).join(" ");
			FRoleRequestId = FMultiChat->setUserRole(nick,MUC_ROLE_NONE,reason);
			if (!FRoleRequestId.isEmpty())
				showMultiChatStatusMessage(tr("Kick user %1 request was sent").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else if (FMultiChat->isOpen())
				showMultiChatStatusMessage(tr("Failed to send kick user %1 request").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <room nick> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/ban ") || AText=="/ban")
	{
		QStringList params = AText.split(" ");
		QString nick = params.value(1);
		if (!nick.isEmpty())
		{
			QString reason = QStringList(params.mid(2)).join(" ");
			FAffilRequestId = FMultiChat->setUserAffiliation(nick,MUC_AFFIL_OUTCAST,reason);
			if (!FAffilRequestId.isEmpty())
				showMultiChatStatusMessage(tr("Ban user %1 request was sent").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else if (FMultiChat->isOpen())
				showMultiChatStatusMessage(tr("Failed to send ban user %1 request").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <user nick> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/invite ") || AText=="/invite")
	{
		QStringList params = AText.split(" ");
		Jid userJid = params.value(1);
		if (userJid.isValid())
		{
			QString reason = QStringList(params.mid(2)).join(" ");
			if (FMultiChat->sendInvitation(userJid,reason))
				showMultiChatStatusMessage(tr("Invitation was sent to user %1").arg(userJid.full()),IMessageStyleContentOptions::TypeNotification);
			else if (FMultiChat->isOpen())
				showMultiChatStatusMessage(tr("Failed to send invitation to user %1").arg(userJid.full()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <user jid> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/join ") || AText=="/join")
	{
		QStringList params = AText.split(" ");
		QString room = params.value(1);
		if (!room.isEmpty())
		{
			QString password = QStringList(params.mid(2)).join(" ");
			QString roomJid = !room.contains('@') ? room + "@" + FMultiChat->roomJid().domain() : room;
			FMultiChatManager->showJoinMultiChatWizard(streamJid(),Jid::fromUserInput(roomJid),FMultiChat->nickName(),password);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <room name> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/msg ") || AText=="/msg")
	{
		QStringList params = AText.split(" ");
		QString nick = params.value(1);
		if (!nick.isEmpty() && params.count()>2)
		{
			Message message;
			message.setBody(QStringList(params.mid(2)).join(" "));
			if (FMultiChat->sendMessage(message,nick))
				showMultiChatStatusMessage(tr("Private message was sent to user %1").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else if (FMultiChat->isOpen())
				showMultiChatStatusMessage(tr("Failed to send private message to user %1").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <room nick> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/nick ") || AText=="/nick")
	{
		QStringList params = AText.split(" ");
		QString nick = QStringList(params.mid(1)).join(" ");
		if (!nick.isEmpty())
		{
			if (FMultiChat->setNickName(nick))
				showMultiChatStatusMessage(tr("You nickname was changed to %1").arg(nick),IMessageStyleContentOptions::TypeNotification);
			else
				showMultiChatStatusMessage(tr("Failed to change your nickname to %1").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		else
		{
			showMultiChatStatusMessage(tr("Required parameter <new nick> is not specified"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/topic ") || AText=="/topic")
	{
		QStringList params = AText.split(" ");
		QString subject = QStringList(params.mid(1)).join(" ");

		if (FMultiChat->sendSubject(subject))
			showMultiChatStatusMessage(tr("Change subject request was sent"),IMessageStyleContentOptions::TypeNotification);
		else
			showMultiChatStatusMessage(tr("Failed to send change subject request"),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);

		hasCommand = true;
	}
	else if (AText.startsWith("/part ") || AText.startsWith("/leave ") || AText=="/part" || AText=="/leave")
	{
		QStringList params = AText.split(" ");
		QString status = QStringList(params.mid(1)).join(" ");
		
		FMultiChat->sendPresence(IPresence::Offline,status,0);
		exitAndDestroy(QString::null);
		
		hasCommand = true;
	}
	else if (AText == "/help")
	{
		showMultiChatStatusMessage(tr(
			"Supported list of commands: \n"
			" /ban <roomnick> [comment] \n"
			" /invite <jid> [comment] \n"
			" /join <roomname> [pass] \n"
			" /kick <roomnick> [comment] \n"
			" /msg <roomnick> <foo> \n"
			" /nick <newnick> \n"
			" /leave [comment] \n"
			" /topic <foo>"),
			IMessageStyleContentOptions::TypeNotification
		);

		hasCommand = true;
	}
	return hasCommand;
}

void MultiUserChatWindow::updateRecentItemActiveTime(IMessageChatWindow *AWindow)
{
	if (FRecentContacts)
	{
		IRecentItem recentItem;
		recentItem.streamJid = streamJid();
		if (AWindow == NULL)
		{
			recentItem.type = REIT_CONFERENCE;
			recentItem.reference = contactJid().pBare();
		}
		else
		{
			recentItem.type = REIT_CONFERENCE_PRIVATE;
			recentItem.reference = AWindow->contactJid().pFull();
		}
		FRecentContacts->setItemActiveTime(recentItem);
	}
}

void MultiUserChatWindow::showDateSeparator(IMessageViewWidget *AView, const QDateTime &ADateTime)
{
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
	{
		QDate sepDate = ADateTime.date();
		WindowStatus &wstatus = FWindowStatus[AView];
		if (FMessageStyleManager && sepDate.isValid() && wstatus.lastDateSeparator!=sepDate)
		{
			IMessageStyleContentOptions options;
			options.kind = IMessageStyleContentOptions::KindStatus;
			if (wstatus.createTime > ADateTime)
				options.type |= IMessageStyleContentOptions::TypeHistory;
			options.status = IMessageStyleContentOptions::StatusDateSeparator;
			options.direction = IMessageStyleContentOptions::DirectionIn;
			options.time.setDate(sepDate);
			options.time.setTime(QTime(0,0));
			options.timeFormat = " ";
			wstatus.lastDateSeparator = sepDate;
			AView->appendText(FMessageStyleManager->dateSeparator(sepDate),options);
		}
	}
}

void MultiUserChatWindow::showHTMLStatusMessage(IMessageViewWidget *AView, const QString &AHtml, int AType, int AStatus, const QDateTime &ATime)
{
	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindStatus;
	options.type |= AType;
	options.status = AStatus;
	options.direction = IMessageStyleContentOptions::DirectionIn;

	options.time = ATime;
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyleManager->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyleManager->timeFormat(options.time);

	showDateSeparator(FViewWidget,options.time);
	AView->appendHtml(AHtml,options);
}

bool MultiUserChatWindow::isMentionMessage(const Message &AMessage) const
{
	QString message = AMessage.body();
	QString nick = FMultiChat->nickName();

	// QString::indexOf(QRegExp) will not work if nick ends with '+'
	if (!nick.isEmpty() && !nick.at(nick.size()-1).isLetterOrNumber())
	{
		message.replace(nick,nick+'z');
		nick += 'z';
	}

	QRegExp mention(QString("\\b%1\\b").arg(QRegExp::escape(nick)));
	return message.indexOf(mention)>=0;
}

void MultiUserChatWindow::setMultiChatMessageStyle()
{
	if (FMessageStyleManager)
	{
		LOG_STRM_DEBUG(streamJid(),QString("Changing message style for multi chat window, room=%1").arg(contactJid().bare()));
		IMessageStyleOptions soptions = FMessageStyleManager->styleOptions(Message::GroupChat);
		if (FViewWidget->messageStyle()==NULL || !FViewWidget->messageStyle()->changeOptions(FViewWidget->styleWidget(),soptions,true))
		{
			IMessageStyle *style = FMessageStyleManager->styleForOptions(soptions);
			FViewWidget->setMessageStyle(style,soptions);
		}
		FWindowStatus[FViewWidget].lastDateSeparator = QDate();
	}
}

void MultiUserChatWindow::showMultiChatTopic(const QString &ATopic, const QString &ANick)
{
	Q_UNUSED(ANick);

	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindTopic;
	options.type |= IMessageStyleContentOptions::TypeGroupchat;
	options.direction = IMessageStyleContentOptions::DirectionIn;

	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyleManager->timeFormat(options.time);

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendText(tr("Subject: %1").arg(ATopic),options);
}

void MultiUserChatWindow::showMultiChatStatusMessage(const QString &AMessage, int AType, int AStatus, bool ADontSave, const QDateTime &ATime)
{
	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindStatus;
	options.type |= AType;
	options.status = AStatus;
	options.direction = IMessageStyleContentOptions::DirectionIn;

	options.time = ATime;
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyleManager->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyleManager->timeFormat(options.time);

	if (!ADontSave && FMessageArchiver && Options::node(OPV_MUC_GROUPCHAT_ARCHIVESTATUS).value().toBool())
		FMessageArchiver->saveNote(FMultiChat->streamJid(), FMultiChat->roomJid(), AMessage);

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendText(AMessage,options);
}

bool MultiUserChatWindow::showMultiChatStatusCodes(const QList<int> &ACodes, const QString &ANick, const QString &AMessage)
{
	if (!ACodes.isEmpty())
	{
		QList< QPair<QString,int> > statuses;

		if (ACodes.contains(MUC_SC_NON_ANONYMOUS))
			statuses.append(qMakePair<QString,int>(tr("This conference is non-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_AFFIL_CHANGED))
			statuses.append(qMakePair<QString,int>(tr("User %1 affiliation changed while not in the conference").arg(ANick),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_MEMBERS_SHOW))
			statuses.append(qMakePair<QString,int>(tr("Conference now shows unavailable members"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_MEMBERS_HIDE))
			statuses.append(qMakePair<QString,int>(tr("Conference now does not show unavailable members"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_CONFIG_CHANGED))
			statuses.append(qMakePair<QString,int>(tr("Conference configuration change has occurred"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_LOGGING_ENABLED))
			statuses.append(qMakePair<QString,int>(tr("Conference logging is now enabled"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_LOGGING_DISABLED))
			statuses.append(qMakePair<QString,int>(tr("Conference logging is now disabled"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_NON_ANONYMOUS))
			statuses.append(qMakePair<QString,int>(tr("The conference is now non-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_SEMI_ANONYMOUS))
			statuses.append(qMakePair<QString,int>(tr("The conference is now semi-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_ROOM_CREATED))
			statuses.append(qMakePair<QString,int>(tr("A new conference has been created"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_AFFIL_CHANGE))
			statuses.append(qMakePair<QString,int>(tr("User %1 has been removed from the conference because of an affiliation change").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		if (ACodes.contains(MUC_SC_MEMBERS_ONLY))
			statuses.append(qMakePair<QString,int>(tr("User %1 has been removed from the conference because the conference has been changed to members-only").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		if (ACodes.contains(MUC_SC_SYSTEM_SHUTDOWN))
			statuses.append(qMakePair<QString,int>(tr("User %1 is being removed from the conference because of a system shutdown").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		// Do not notify
		if (ACodes.contains(MUC_SC_SELF_PRESENCE))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		// Do not notify
		if (ACodes.contains(MUC_SC_NICK_ASSIGNED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		// Notified by IMultiUserChat::userChanged()
		if (ACodes.contains(MUC_SC_NICK_CHANGED))
			statuses.append(qMakePair<QString,int>(QString::null,0));
		
		// Notified by IMultiUserChat::userKicked()
		if (ACodes.contains(MUC_SC_USER_KICKED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		// Notified by IMultiUserChat::userBanned()
		if (ACodes.contains(MUC_SC_USER_BANNED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		for (QList< QPair<QString,int> >::const_iterator it=statuses.constBegin(); it!=statuses.constEnd(); ++it)
		{
			QString status = it->first;
			if (!status.isEmpty())
			{
				if (!AMessage.isEmpty())
					status += QString(" (%1)").arg(AMessage);
				showMultiChatStatusMessage(status,it->second);
			}
		}

		return !statuses.isEmpty();
	}
	return false;
}

void MultiUserChatWindow::showMultiChatUserMessage(const Message &AMessage, const QString &ANick)
{
	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindMessage;

	options.type |= IMessageStyleContentOptions::TypeGroupchat;
	if (AMessage.isDelayed())
		options.type |= IMessageStyleContentOptions::TypeHistory;

	options.time = AMessage.dateTime();
	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		options.timeFormat = FMessageStyleManager->timeFormat(options.time,options.time);
	else
		options.timeFormat = FMessageStyleManager->timeFormat(options.time);

	options.senderName = Qt::escape(ANick);
	options.senderId = options.senderName;

	IMultiUser *user = FMultiChat->nickName()!=ANick ? FMultiChat->findUser(ANick) : FMultiChat->mainUser();
	if (user)
	{
		options.senderAvatar = FMessageStyleManager->contactAvatar(user->userJid());
		options.senderIcon = FMessageStyleManager->contactIcon(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false);
	}
	else
	{
		options.senderIcon = FMessageStyleManager->contactIcon(Jid::null,IPresence::Offline,SUBSCRIPTION_BOTH,false);
	}

	if (FMultiChat->nickName() != ANick)
	{
		if (isMentionMessage(AMessage))
			options.type |= IMessageStyleContentOptions::TypeMention;
		options.direction = IMessageStyleContentOptions::DirectionIn;
	}
	else
	{
		options.direction = IMessageStyleContentOptions::DirectionOut;
	}

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendMessage(AMessage,options);
}

void MultiUserChatWindow::requestMultiChatHistory()
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
			LOG_STRM_INFO(streamJid(),QString("Load multi chat history request sent, room=%1, id=%2").arg(request.with.bare(),reqId));
			showMultiChatStatusMessage(tr("Loading history..."),IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusEmpty,true);
			FHistoryRequests.insert(reqId,NULL);
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to send multi chat history load request, room=%1").arg(request.with.bare()));
		}
	}
}

void MultiUserChatWindow::updateMultiChatWindow()
{
	FInfoWidget->setFieldValue(IMessageInfoWidget::Name,FMultiChat->roomTitle());

	QIcon statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(contactJid(),FMultiChat->roomPresence().show,SUBSCRIPTION_BOTH,false) : QIcon();
	FInfoWidget->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);
	FInfoWidget->setFieldValue(IMessageInfoWidget::StatusText,FMultiChat->subject());

	QIcon tabIcon = statusIcon;
	if (FTabPageNotifier && FTabPageNotifier->activeNotify()>0)
		tabIcon = FTabPageNotifier->notifyById(FTabPageNotifier->activeNotify()).icon;

	setWindowIcon(tabIcon);
	setWindowIconText(QString("%1 (%2)").arg(FMultiChat->roomName()).arg(FUsers.count()));
	setWindowTitle(tr("%1 - Conference").arg(FMultiChat->roomTitle()));

	emit tabPageChanged();
}

void MultiUserChatWindow::removeMultiChatActiveMessages()
{
	if (FMessageProcessor)
	{
		foreach(int messageId, FActiveMessages)
			FMessageProcessor->removeMessageNotify(messageId);
	}
	FActiveMessages.clear();
}

IMessageChatWindow *MultiUserChatWindow::getPrivateChatWindow(const Jid &AContactJid)
{
	IMessageChatWindow *window = findPrivateChatWindow(AContactJid);
	if (window == NULL)
	{
		IMultiUser *user = FMultiChat->findUser(AContactJid.resource());
		if (user!=NULL && user!=FMultiChat->mainUser())
		{
			window = FMessageWidgets!=NULL ? FMessageWidgets->getChatWindow(streamJid(),AContactJid) : NULL;
			if (window)
			{
				LOG_STRM_INFO(streamJid(),QString("Private chat window created, room=%1, user=%2").arg(contactJid().bare(),AContactJid.resource()));

				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onPrivateChatWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onPrivateChatWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onPrivateChatWindowDestroyed()));
				connect(window->infoWidget()->instance(),SIGNAL(contextMenuRequested(Menu *)),SLOT(onPrivateChatContextMenuRequested(Menu *)));
				connect(window->infoWidget()->instance(),SIGNAL(toolTipsRequested(QMap<int,QString> &)),SLOT(onPrivateChatToolTipsRequested(QMap<int,QString> &)));
				connect(window->viewWidget()->instance(),SIGNAL(contentAppended(const QString &, const IMessageStyleContentOptions &)),
					SLOT(onPrivateChatContentAppended(const QString &, const IMessageStyleContentOptions &)));
				connect(window->viewWidget()->instance(),SIGNAL(messageStyleOptionsChanged(const IMessageStyleOptions &, bool)),
					SLOT(onPrivateChatMessageStyleOptionsChanged(const IMessageStyleOptions &, bool)));
				connect(window->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onPrivateChatNotifierActiveNotifyChanged(int)));

				FPrivateChatWindows.append(window);
				FWindowStatus[window->viewWidget()].createTime = QDateTime::currentDateTime();

				Action *clearAction = new Action(window->instance());
				clearAction->setToolTip(tr("Clear window"));
				clearAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
				connect(clearAction,SIGNAL(triggered(bool)),SLOT(onPrivateChatClearWindowActionTriggered(bool)));
				window->toolBarWidget()->toolBarChanger()->insertAction(clearAction, TBG_MWTBW_CLEAR_WINDOW);

				updatePrivateChatWindow(window);
				setPrivateChatMessageStyle(window);
				requestPrivateChatHistory(window);
				emit privateChatWindowCreated(window);
			}
			else
			{
				LOG_STRM_ERROR(streamJid(),QString("Failed to create private chat window, room=%1, user=%2: Instance is not created").arg(contactJid().bare(),AContactJid.resource()));
			}
		}
		else if (user == NULL)
		{
			REPORT_ERROR("Failed to create private chat window: User not found");
		}
	}
	return window;
}

void MultiUserChatWindow::setPrivateChatMessageStyle(IMessageChatWindow *AWindow)
{
	if (FMessageStyleManager)
	{
		LOG_STRM_DEBUG(streamJid(),QString("Changing message style for private chat window, room=%1, user=%2").arg(contactJid().bare(),AWindow->contactJid().resource()));
		IMessageStyleOptions soptions = FMessageStyleManager->styleOptions(Message::Chat);
		if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true))
		{
			IMessageStyle *style = FMessageStyleManager->styleForOptions(soptions);
			AWindow->viewWidget()->setMessageStyle(style,soptions);
		}
		FWindowStatus[AWindow->viewWidget()].lastDateSeparator = QDate();
	}
}

void MultiUserChatWindow::fillPrivateChatContentOptions(IMessageChatWindow *AWindow, IMessageStyleContentOptions &AOptions) const
{
	IMultiUser *user = AOptions.direction==IMessageStyleContentOptions::DirectionIn ? FMultiChat->findUser(AWindow->contactJid().resource()) : FMultiChat->mainUser();
	if (user)
	{
		AOptions.senderAvatar = FMessageStyleManager->contactAvatar(user->userJid());
		AOptions.senderIcon = FMessageStyleManager->contactIcon(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false);
	}

	if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		AOptions.timeFormat = FMessageStyleManager->timeFormat(AOptions.time,AOptions.time);
	else
		AOptions.timeFormat = FMessageStyleManager->timeFormat(AOptions.time);

	if (AOptions.direction == IMessageStyleContentOptions::DirectionIn)
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

void MultiUserChatWindow::showPrivateChatStatusMessage(IMessageChatWindow *AWindow, const QString &AMessage, int AStatus, const QDateTime &ATime)
{
	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindStatus;
	options.status = AStatus;
	options.direction = IMessageStyleContentOptions::DirectionIn;
	options.time = ATime;

	fillPrivateChatContentOptions(AWindow,options);
	showDateSeparator(AWindow->viewWidget(),options.time);
	AWindow->viewWidget()->appendText(AMessage,options);
}

void MultiUserChatWindow::showPrivateChatMessage(IMessageChatWindow *AWindow, const Message &AMessage)
{
	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindMessage;
	options.time = AMessage.dateTime();

	if (options.time.secsTo(FWindowStatus.value(AWindow->viewWidget()).createTime)>HISTORY_TIME_DELTA)
		options.type |= IMessageStyleContentOptions::TypeHistory;

	if (AMessage.data(MDR_MESSAGE_DIRECTION).toInt() == IMessageProcessor::DirectionOut)
		options.direction = IMessageStyleContentOptions::DirectionOut;
	else
		options.direction = IMessageStyleContentOptions::DirectionIn;

	fillPrivateChatContentOptions(AWindow,options);
	showDateSeparator(AWindow->viewWidget(),options.time);
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

void MultiUserChatWindow::requestPrivateChatHistory(IMessageChatWindow *AWindow)
{
	if (FMessageArchiver && Options::node(OPV_MESSAGES_LOADHISTORY).value().toBool() && !FHistoryRequests.values().contains(AWindow))
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
			LOG_STRM_INFO(streamJid(),QString("Load private chat history request sent, room=%1, user=%2, id=%3").arg(request.with.bare(),AWindow->contactJid().resource(),reqId));
			showPrivateChatStatusMessage(AWindow,tr("Loading history..."));
			FHistoryRequests.insert(reqId,AWindow);
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to send private chat history load request, room=%1, user=%2").arg(request.with.bare(),AWindow->contactJid().resource()));
		}
	}
}

void MultiUserChatWindow::updatePrivateChatWindow(IMessageChatWindow *AWindow)
{
	IMultiUser *user = FMultiChat->findUser(AWindow->contactJid().resource());
	if (user)
	{
		if (FAvatars)
		{
			QString avatar = FAvatars->avatarHash(user->userJid(), true);
			if (FAvatars->hasAvatar(avatar))
				AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Avatar,avatar);
			else
				AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Avatar,FAvatars->emptyAvatarImage(FAvatars->avatarSize(IAvatars::AvatarSmall)));
		}

		QString name = tr("[%1] in [%2]").arg(user->nick(),FMultiChat->roomName());
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Name,name);

		QIcon statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false) : QIcon();
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusText,user->presence().status);

		QIcon tabIcon = statusIcon;
		if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
			tabIcon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;

		AWindow->updateWindow(tabIcon,name,tr("%1 - Private Chat").arg(name),QString::null);
	}
}

void MultiUserChatWindow::removePrivateChatActiveMessages(IMessageChatWindow *AWindow)
{
	if (FActiveChatMessages.contains(AWindow))
	{
		foreach(int messageId, FActiveChatMessages.values(AWindow))
		{
			if (FMessageProcessor)
				FMessageProcessor->removeMessageNotify(messageId);
			FUsersView->removeItemNotify(FActiveChatMessageNotify.take(messageId));
		}
		FActiveChatMessages.remove(AWindow);
	}
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

	if (!FStateLoaded)
		loadWindowState();

	if (FEditWidget)
		FEditWidget->textEdit()->setFocus();
	
	if (isActiveTabPage())
		emit tabPageActivated();
}

void MultiUserChatWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	saveWindowState();

	if (Options::node(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE).value().toBool() && !Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		exitAndDestroy(QString::null);

	QMainWindow::closeEvent(AEvent);

	emit tabPageClosed();
}

bool MultiUserChatWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (FUsersView && AObject==FUsersView->instance()->viewport())
	{
		if (AEvent->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(AEvent);
			if (FEditWidget!=NULL && mouseEvent!=NULL)
			{
				QStandardItem *userItem = FUsersView->itemFromIndex(FUsersView->instance()->indexAt(mouseEvent->pos()));
				if(mouseEvent->button()==Qt::MidButton && userItem!=NULL)
				{
					QString sufix = FEditWidget->textEdit()->textCursor().atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFFIX).value().toString() : QString(" ");
					FEditWidget->textEdit()->textCursor().insertText(userItem->text() + sufix);
					FEditWidget->textEdit()->setFocus();
					AEvent->accept();
					return true;
				}
			}
		}
	}
	return QMainWindow::eventFilter(AObject,AEvent);
}

void MultiUserChatWindow::onMultiChatStateChanged(int AState)
{
	QAction *enterHandler = FToolBarWidget->toolBarChanger()->actionHandle(FEnterRoom);
	if (enterHandler)
		enterHandler->setVisible(AState == IMultiUserChat::Closed);

	if (AState == IMultiUserChat::Opened)
	{
		if (FStanzaProcessor)
		{
			IStanzaHandle shandle;
			shandle.handler = this;
			shandle.order = qMin(SHO_MI_MULTIUSERCHAT,SHO_PI_MULTIUSERCHAT)-1;
			shandle.direction = IStanzaHandle::DirectionIn;
			shandle.streamJid = FMultiChat->streamJid();
			shandle.conditions.append("/iq");
			shandle.conditions.append("/message");
			shandle.conditions.append("/presence");
			FSHIAnyStanza = FStanzaProcessor->insertStanzaHandle(shandle);
		}

		updateRecentItemActiveTime(NULL);
		showMultiChatStatusMessage(tr("You have joined the conference"),IMessageStyleContentOptions::TypeEvent,IMessageStyleContentOptions::StatusOnline);

		if (FMultiChat->mainUser()->role() == MUC_ROLE_VISITOR)
		{
			QUrl url;
			url.setScheme(MUC_URL_SCHEME);
			url.setPath(FMultiChat->mainUser()->userJid().full());
			url.setFragment(MUC_URL_REQUESTVOICE);

			QString html = tr("You have no voice in this conference, %1").arg(QString("<a href='%1'>%2</a>").arg(url.toString(),tr("Request Voice")));
			showHTMLStatusMessage(FViewWidget,html,IMessageStyleContentOptions::TypeNotification);
		}
		else if (FMultiChat->statusCodes().contains(MUC_SC_ROOM_CREATED))
		{
			FInitializeConfig = true;
			FConfigLoadRequestId = FMultiChat->loadRoomConfig();
		}
	}
	else if (AState == IMultiUserChat::Closed)
	{
		if (FStanzaProcessor)
		{
			FStanzaProcessor->removeStanzaHandle(FSHIAnyStanza);
			FSHIAnyStanza = -1;
		}

		if (!FDestroyOnChatClosed)
		{
			IMultiUserChatHistory history;
			history.since = FLastStanzaTime;
			FMultiChat->setHistoryScope(history);

			if (FMultiChat->roomPresence().show == IPresence::Error)
				showMultiChatStatusMessage(tr("You have left the conference due to error: %1").arg(FMultiChat->roomPresence().status),IMessageStyleContentOptions::TypeEvent,IMessageStyleContentOptions::StatusError);
			else
				showMultiChatStatusMessage(tr("You have left the conference"),IMessageStyleContentOptions::TypeEvent,IMessageStyleContentOptions::StatusOffline);

			updateMultiChatWindow();
		}
		else
		{
			deleteLater();
		}
	}
}

void MultiUserChatWindow::onMultiChatRoomTitleChanged(const QString &ATitle)
{
	Q_UNUSED(ATitle);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onMultiChatRequestFailed(const QString &AId, const XmppError &AError)
{
	if (AId	== FRoleRequestId)
		showMultiChatStatusMessage(tr("Failed to change user role: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
	else if (AId == FAffilRequestId)
		showMultiChatStatusMessage(tr("Failed to change user affiliation: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
	else if (AId == FConfigLoadRequestId)
		showMultiChatStatusMessage(tr("Failed to load conference configuration: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
	else if (AId == FConfigUpdateRequestId)
		showMultiChatStatusMessage(tr("Failed to update conference configuration: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
	else if (AId == FDestroyRequestId)
		showMultiChatStatusMessage(tr("Failed to destroy this conference: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
}

void MultiUserChatWindow::onMultiChatPresenceChanged(const IPresenceItem &APresence)
{
	Q_UNUSED(APresence);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onMultiChatInvitationDeclined(const Jid &AContactJid, const QString &AReason)
{
	QString nick = contactJid().pBare()==AContactJid.pBare() ? AContactJid.resource() : AContactJid.uFull();
	showMultiChatStatusMessage(tr("User %1 has declined your invite to this conference. %2").arg(nick).arg(AReason),IMessageStyleContentOptions::TypeNotification);
}

void MultiUserChatWindow::onMultiChatUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore)
{
	if (AData == MUDR_PRESENCE)
	{
		QString enterMessage;
		QString statusMessage;
		IPresenceItem presence = AUser->presence();

		if (presence.show!=IPresence::Offline && presence.show!=IPresence::Error)
		{
			QString show = FStatusChanger ? FStatusChanger->nameByShow(presence.show) : QString::null;
			if (!FUsers.contains(AUser))
			{
				UserStatus &userStatus = FUsers[AUser];
				userStatus.lastStatusShow = presence.status+show;

				if (FMultiChat->isOpen() && AUser!=FMultiChat->mainUser() && Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS).value().toBool())
				{
					if (AUser->realJid().isValid())
						enterMessage = tr("%1 <%2> has joined").arg(AUser->nick()).arg(AUser->realJid().uFull());
					else
						enterMessage = tr("%1 has joined").arg(AUser->nick());
					if (!presence.status.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
						enterMessage += QString(" - [%1] %2").arg(show).arg(presence.status);
					showMultiChatStatusMessage(enterMessage,IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusJoined);
				}

				refreshCompleteNicks();
				updateMultiChatWindow();
			}
			else
			{
				UserStatus &userStatus = FUsers[AUser];
				if (Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool() && userStatus.lastStatusShow!=presence.status+show)
				{
					statusMessage = tr("%1 changed status to [%2] %3").arg(AUser->nick()).arg(show).arg(presence.status);
					showMultiChatStatusMessage(statusMessage);
				}
				userStatus.lastStatusShow = presence.status+show;
			}

			showMultiChatStatusCodes(FMultiChat->statusCodes(),AUser->nick());
		}
		else if (FUsers.contains(AUser))
		{
			if (!showMultiChatStatusCodes(FMultiChat->statusCodes(),AUser->nick()))
			{
				if (FMultiChat->isOpen() && AUser!=FMultiChat->mainUser() && Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS).value().toBool())
				{
					if (AUser->realJid().isValid())
						enterMessage = tr("%1 <%2> has left").arg(AUser->nick()).arg(AUser->realJid().uBare());
					else
						enterMessage = tr("%1 has left").arg(AUser->nick());
					if (!presence.status.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
						enterMessage += QString(" - %1").arg(presence.status);
					showMultiChatStatusMessage(enterMessage,IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusLeft);
				}
			}
			FUsers.remove(AUser);

			refreshCompleteNicks();
			updateMultiChatWindow();
		}

		IMessageChatWindow *window = findPrivateChatWindow(AUser->userJid());
		if (window)
		{
			if (FUsers.contains(AUser) || !FDestroyTimers.contains(window))
			{
				if (!enterMessage.isEmpty())
					showPrivateChatStatusMessage(window,enterMessage,presence.show!=IPresence::Offline && presence.show!=IPresence::Error ? IMessageStyleContentOptions::StatusJoined : IMessageStyleContentOptions::StatusLeft);
				if (!statusMessage.isEmpty())
					showPrivateChatStatusMessage(window,statusMessage);
				updatePrivateChatWindow(window);
			}
			else
			{
				window->instance()->deleteLater();
			}
		}
	}
	else if (AData == MUDR_NICK)
	{
		if (AUser == FMultiChat->mainUser())
		{
			updateMultiChatWindow();
		}
		else if (FUsers.contains(AUser))
		{
			Jid breforeUserJid = AUser->userJid();
			breforeUserJid.setResource(ABefore.toString());
			
			IMessageChatWindow *window = findPrivateChatWindow(breforeUserJid);
			if (window)
			{
				window->address()->appendAddress(streamJid(),AUser->userJid());
				window->address()->removeAddress(streamJid(),breforeUserJid);
				window->address()->setAddress(streamJid(),AUser->userJid());
				updatePrivateChatWindow(window);
			}
		}

		refreshCompleteNicks();
		showMultiChatStatusMessage(tr("%1 changed nick to %2").arg(ABefore.toString()).arg(AUser->nick()),IMessageStyleContentOptions::TypeEvent);
	}
	else if (AData == MUDR_ROLE)
	{
		if (AUser->role()!=MUC_ROLE_NONE && ABefore!=MUC_ROLE_NONE)
			showMultiChatStatusMessage(tr("%1 changed role from %2 to %3").arg(AUser->nick()).arg(ABefore.toString()).arg(AUser->role()),IMessageStyleContentOptions::TypeEvent);
	}
	else if (AData == MUDR_AFFILIATION)
	{
		if (FUsers.contains(AUser))
			showMultiChatStatusMessage(tr("%1 changed affiliation from %2 to %3").arg(AUser->nick()).arg(ABefore.toString()).arg(AUser->affiliation()),IMessageStyleContentOptions::TypeEvent);
	}
}

void MultiUserChatWindow::onMultiChatVoiceRequestReceived(const Message &AMessage)
{
	if (FDataForms && FMessageProcessor)
	{
		IDataForm form = FDataForms->dataForm(AMessage.stanza().firstElement("x",NS_JABBER_DATA));

		Jid reqJid = FDataForms->fieldValue("muc#jid",form.fields).toString();
		QString reqRole = FDataForms->fieldValue("muc#role",form.fields).toString();
		QString reqNick = FDataForms->fieldValue("muc#roomnick",form.fields).toString();

		IMultiUser *user = FMultiChat->findUser(reqNick);
		if (user!=NULL && user->role()==MUC_ROLE_VISITOR)
		{
			Message message;
			message.setTo(AMessage.to()).setFrom(AMessage.from()).setId(AMessage.id()).setType(AMessage.type());

			Stanza &mstanza = message.stanza();
			QDomElement requestElem = mstanza.addElement("x",DFT_MUC_REQUEST);
			requestElem.appendChild(mstanza.createElement("jid")).appendChild(mstanza.createTextNode(reqJid.full()));
			requestElem.appendChild(mstanza.createElement("role")).appendChild(mstanza.createTextNode(reqRole));
			requestElem.appendChild(mstanza.createElement("roomnick")).appendChild(mstanza.createTextNode(reqNick));

			FMessageProcessor->sendMessage(streamJid(),message,IMessageProcessor::DirectionIn);
		}
	}
}

void MultiUserChatWindow::onMultiChatSubjectChanged(const QString &ANick, const QString &ASubject)
{
	showMultiChatTopic(ASubject,ANick);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onMultiChatUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->findUser(ANick);
	Jid realJid = user!=NULL ? user->realJid() : Jid::null;

	showMultiChatStatusMessage(tr("User %1 has been kicked from the conference%2 %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uBare()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by user %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageStyleContentOptions::TypeEvent);

	if (Options::node(OPV_MUC_GROUPCHAT_REJOINAFTERKICK).value().toBool() && user==FMultiChat->mainUser())
		QTimer::singleShot(REJOIN_AFTER_KICK_MSEC,this,SLOT(onAutoRejoinAfterKick()));
}

void MultiUserChatWindow::onMultiChatUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->findUser(ANick);
	Jid realJid = user!=NULL ? user->realJid() : Jid::null;

	showMultiChatStatusMessage(tr("User %1 has been banned in the conference%2 %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uFull()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageStyleContentOptions::TypeEvent);
}

void MultiUserChatWindow::onMultiChatRoomConfigLoaded(const QString &AId, const IDataForm &AForm)
{
	if (FDataForms && AId==FConfigLoadRequestId)
	{
		IDataForm localizedForm = FDataForms->localizeForm(AForm);
		localizedForm.title = QString("%1 (%2)").arg(localizedForm.title, FMultiChat->roomJid().uBare());

		IDataDialogWidget *dialog = FDataForms->dialogWidget(localizedForm,this);
		connect(dialog->instance(),SIGNAL(accepted()),SLOT(onRoomConfigFormDialogAccepted()));
		connect(dialog->instance(),SIGNAL(rejected()),SLOT(onRoomConfigFormDialogRejected()));
		connect(FMultiChat->instance(),SIGNAL(stateChanged(int)),dialog->instance(),SLOT(reject()));
		dialog->instance()->show();
	}
}

void MultiUserChatWindow::onMultiChatRoomConfigUpdated(const QString &AId, const IDataForm &AForm)
{
	Q_UNUSED(AForm);
	if (AId == FConfigUpdateRequestId)
	{
		FInitializeConfig = false;
		showMultiChatStatusMessage(tr("Conference configuration accepted"),IMessageStyleContentOptions::TypeNotification);
	}
}

void MultiUserChatWindow::onMultiChatRoomDestroyed(const QString &AId, const QString &AReason)
{
	if (AId == FDestroyRequestId)
		showMultiChatStatusMessage(tr("This conference was destroyed by owner. %1").arg(AReason),IMessageStyleContentOptions::TypeNotification);
}

void MultiUserChatWindow::onMultiChatWindowActivated()
{
	LOG_STRM_DEBUG(streamJid(),QString("Multi chat window activated, room=%1").arg(contactJid().bare()));
	removeMultiChatActiveMessages();
}

void MultiUserChatWindow::onMultiChatNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onMultiChatEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked)
{
	if (FMultiChat->isOpen() && AKeyEvent->modifiers()+AKeyEvent->key() == NICK_MENU_KEY)
	{
		QTextEdit *textEdit = FEditWidget->textEdit();
		QTextCursor cursor = textEdit->textCursor();
		if (FCompleteIt == FCompleteNicks.constEnd())
		{
			while (cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor))
			{
				if (cursor.selectedText().at(0).isSpace())
				{
					cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);
					break;
				}
			}
			FStartCompletePos = cursor.position();
			FCompleteNickStarts = cursor.selectedText().toLower();
			refreshCompleteNicks();
		}
		else
		{
			cursor.setPosition(FStartCompletePos, QTextCursor::KeepAnchor);
		}

		QString suffix = cursor.atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFFIX).value().toString() : QString(" ");
		if (FCompleteNicks.count() > 1)
		{
			if (!Options::node(OPV_MUC_GROUPCHAT_REFERENUMERATION).value().toBool())
			{
				Menu *nickMenu = new Menu(this);
				nickMenu->setAttribute(Qt::WA_DeleteOnClose,true);
				foreach(const QString &nick, FCompleteNicks)
				{
					IMultiUser *user = FMultiChat->findUser(nick);
					if (user)
					{
						Action *action = new Action(nickMenu);
						action->setText(user->nick());
						action->setIcon(FUsersView->findUserItem(user)->icon());
						action->setData(ADR_USER_NICK,user->nick());
						connect(action,SIGNAL(triggered(bool)),SLOT(onNickCompleteMenuActionTriggered(bool)));
						nickMenu->addAction(action,AG_DEFAULT,true);
					}
				}
				nickMenu->popup(textEdit->viewport()->mapToGlobal(textEdit->cursorRect().topLeft()));
			}
			else
			{
				FCompleteNickLast = *FCompleteIt;
				cursor.insertText(*FCompleteIt + suffix);

				if (++FCompleteIt == FCompleteNicks.constEnd())
					FCompleteIt = FCompleteNicks.constBegin();
			}
		}
		else if (!FCompleteNicks.isEmpty())
		{
			FCompleteNickLast = *FCompleteIt;
			cursor.insertText(FCompleteNicks.first() + suffix);
		}

		AHooked = true;
	}
	else
	{
		FCompleteIt = FCompleteNicks.constEnd();
	}
}

void MultiUserChatWindow::onMultiChatUserItemNotifyActivated(int ANotifyId)
{
	int messageId = FActiveChatMessageNotify.key(ANotifyId);
	if (messageId > 0)
		messageShowWindow(messageId);
}

void MultiUserChatWindow::onMultiChatUserItemDoubleClicked(const QModelIndex &AIndex)
{
	QStandardItem *item = FUsersView->itemFromIndex(AIndex);
	int notifyId = FUsersView->itemNotifies(item).value(0);
	if (notifyId > 0)
		FUsersView->activateItemNotify(notifyId);
	else if (item->data(MUDR_KIND).toInt() == MUIK_USER)
		openPrivateChatWindow(item->data(MUDR_USER_JID).toString());
}

void MultiUserChatWindow::onMultiChatUserItemContextMenu(QStandardItem *AItem, Menu *AMenu)
{
	IMultiUser *user = FUsersView->findItemUser(AItem);
	if (user!=NULL && user!=FMultiChat->mainUser())
		contextMenuForUser(user,AMenu);
}

void MultiUserChatWindow::onMultiChatUserItemToolTips(QStandardItem *AItem, QMap<int,QString> &AToolTips)
{
	IMultiUser *user = FUsersView->findItemUser(AItem);
	if (user)
		toolTipsForUser(user,AToolTips);
}

void MultiUserChatWindow::onMultiChatContentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions)
{
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	if (widget==FViewWidget && FHistoryRequests.values().contains(NULL))
	{
		WindowContent content;
		content.html = AHtml;
		content.options = AOptions;
		FPendingContent[NULL].append(content);
		LOG_STRM_DEBUG(streamJid(),QString("Added pending content to multi chat window, room=%1").arg(contactJid().bare()));
	}
}

void MultiUserChatWindow::onMultiChatMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared)
{
	Q_UNUSED(AOptions);
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	if (widget == FViewWidget)
	{
		if (ACleared)
			FWindowStatus[FViewWidget].lastDateSeparator = QDate();
		LOG_STRM_DEBUG(streamJid(),QString("Multi chat window style options changed, room=%1, cleared=%2").arg(contactJid().bare()).arg(ACleared));
	}
}

void MultiUserChatWindow::onPrivateChatWindowActivated()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		LOG_STRM_DEBUG(streamJid(),QString("Private chat window activated, room=%1, user=%2").arg(contactJid().bare(),window->contactJid().resource()));
		removePrivateChatActiveMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
	}
}

void MultiUserChatWindow::onPrivateChatWindowClosed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window && FMultiChat->findUser(window->contactJid().resource())!=NULL)
	{
		LOG_STRM_DEBUG(streamJid(),QString("Private chat window closed, room=%1, user=%2").arg(contactJid().bare(),window->contactJid().resource()));
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
		LOG_STRM_DEBUG(streamJid(),QString("Destroying private chat window due to it was closed and user quits, room=%1, user=%2").arg(contactJid().bare(),window->contactJid().resource()));
		window->instance()->deleteLater();
	}
}

void MultiUserChatWindow::onPrivateChatWindowDestroyed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (FPrivateChatWindows.contains(window))
	{
		LOG_STRM_INFO(streamJid(),QString("Private chat window destroyed, room=%1, user=%2").arg(contactJid().bare(),window->contactJid().resource()));
		removePrivateChatActiveMessages(window);
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		FPrivateChatWindows.removeAt(FPrivateChatWindows.indexOf(window));
		FWindowStatus.remove(window->viewWidget());
		FPendingMessages.remove(window);
		FPendingContent.remove(window);
		FHistoryRequests.remove(FHistoryRequests.key(window));
		emit privateChatWindowDestroyed(window);
	}
}

void MultiUserChatWindow::onPrivateChatClearWindowActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageChatWindow *window = action!=NULL ? qobject_cast<IMessageChatWindow *>(action->parent()) : NULL;
	if (window)
		window->viewWidget()->clearContent();
}

void MultiUserChatWindow::onPrivateChatContextMenuRequested(Menu *AMenu)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	IMultiUser *user = widget!=NULL ? FMultiChat->findUser(widget->messageWindow()->contactJid().resource()) : NULL;
	if (user)
		contextMenuForUser(user,AMenu);
}

void MultiUserChatWindow::onPrivateChatToolTipsRequested(QMap<int,QString> &AToolTips)
{
	IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
	IMultiUser *user = widget!=NULL ? FMultiChat->findUser(widget->messageWindow()->contactJid().resource()) : NULL;
	if (user)
		toolTipsForUser(user,AToolTips);
}

void MultiUserChatWindow::onPrivateChatNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	IMessageTabPageNotifier *notifier = qobject_cast<IMessageTabPageNotifier *>(sender());
	IMessageChatWindow *window = notifier!=NULL ? qobject_cast<IMessageChatWindow *>(notifier->tabPage()->instance()) : NULL;
	if (window)
		updatePrivateChatWindow(window);
}

void MultiUserChatWindow::onPrivateChatContentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions)
{
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	IMessageChatWindow *window = widget!=NULL ? qobject_cast<IMessageChatWindow *>(widget->messageWindow()->instance()) : NULL;
	if (window && FHistoryRequests.values().contains(window))
	{
		WindowContent content;
		content.html = AHtml;
		content.options = AOptions;
		FPendingContent[window].append(content);
		LOG_STRM_DEBUG(streamJid(),QString("Added pending content to private chat window, room=%1, user=%2").arg(contactJid().bare(),window->contactJid().resource()));
	}
}

void MultiUserChatWindow::onPrivateChatMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared)
{
	Q_UNUSED(AOptions);
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	IMessageChatWindow *window = widget!=NULL ? qobject_cast<IMessageChatWindow *>(widget->messageWindow()->instance()) : NULL;
	if (window)
	{
		if (ACleared)
			FWindowStatus[widget].lastDateSeparator = QDate();
		LOG_STRM_DEBUG(streamJid(),QString("Private chat window style options changed, room=%1, user=%2, cleared=%3").arg(contactJid().bare(),window->contactJid().resource()).arg(ACleared));
	}
}

void MultiUserChatWindow::onRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FChangeNick)
	{
		QString nick = QInputDialog::getText(this,tr("Change Nickname - %1").arg(FMultiChat->roomName()),tr("Enter your new nickname:"),QLineEdit::Normal,FMultiChat->nickName());
		if (!nick.isEmpty())
			FMultiChat->setNickName(nick);
	}
	else if (action == FChangeTopic)
	{
		if (FMultiChat->isOpen())
		{
			QString newSubject = FMultiChat->subject();
			InputTextDialog *dialog = new InputTextDialog(this,tr("Change Topic - %1").arg(FMultiChat->roomName()),tr("Enter new topic:"), newSubject);
			if (dialog->exec() == QDialog::Accepted)
				FMultiChat->sendSubject(newSubject);
		}
	}
	else if (action == FClearChat)
	{
		FViewWidget->clearContent();
	}
	else if (action == FEnterRoom)
	{
		FMultiChat->sendStreamPresence();
	}
	else if (action == FExitRoom)
	{
		exitAndDestroy(QString::null);
	}
	else if (action == FInviteContact)
	{
		if (FMultiChat->isOpen())
		{
			Jid userJid = QInputDialog::getText(this,tr("Invite User - %1").arg(FMultiChat->roomName()),tr("Enter user Jabber ID:"));
			if (userJid.isValid())
			{
				QString reason = tr("Please, enter this conference.");
				reason = QInputDialog::getText(this,tr("Invite User - %1").arg(FMultiChat->roomName()),tr("Enter a message:"),QLineEdit::Normal,reason);
				FMultiChat->sendInvitation(userJid,reason);
			}
		}
	}
	else if (action == FRequestVoice)
	{
		if (FMultiChat->isOpen())
		{
			if (FMultiChat->mainUser()->role() != MUC_ROLE_VISITOR)
				showMultiChatStatusMessage(tr("You already have a voice in the conference"),IMessageStyleContentOptions::TypeNotification);
			else if (!FMultiChat->sendVoiceRequest())
				showMultiChatStatusMessage(tr("Failed to send a request for voice in the conference"),IMessageStyleContentOptions::TypeNotification);
			else
				showMultiChatStatusMessage(tr("Request for voice in the conference was sent"),IMessageStyleContentOptions::TypeNotification);
		}
	}
	else if (action == FEditAffiliations)
	{
		if (FMultiChat->isOpen())
		{
			EditUsersListDialog *dialog = new EditUsersListDialog(FMultiChat,MUC_AFFIL_NONE);
			dialog->show();
		}
	}
	else if (action == FConfigRoom)
	{
		FConfigLoadRequestId = FMultiChat->loadRoomConfig();
	}
	else if (action == FDestroyRoom)
	{
		if (FMultiChat->isOpen())
		{
			bool ok = false;
			QString reason = QInputDialog::getText(this,tr("Destroy Conference - %1").arg(FMultiChat->roomName()),tr("Enter a message:"),QLineEdit::Normal,QString::null,&ok);
			if (ok)
				FDestroyRequestId = FMultiChat->destroyRoom(reason);
		}
	}
	else if (action == FUsersHide)
	{
		if (FUsersHide->isChecked())
		{
			int width = Options::fileValue("muc.mucwindow.users-list-width",tabPageId()).toInt();
			FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE, width>0 ? width : DEFAULT_USERS_LIST_WIDTH);
		}
		else
		{
			int width = FCentralSplitter->handleSize(MUCWW_USERSHANDLE);
			Options::setFileValue(width,"muc.mucwindow.users-list-width",tabPageId());
			FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,0);
		}
	}
}

void MultiUserChatWindow::onNickCompleteMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString nick = action->data(ADR_USER_NICK).toString();
		QTextCursor cursor = FEditWidget->textEdit()->textCursor();
		cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
		QString sufix = cursor.atBlockStart() ? Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFFIX).value().toString() : " ";
		cursor.insertText(nick + sufix);
	}
}

void MultiUserChatWindow::onOpenPrivateChatWindowActionTriggered(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUser *user = FMultiChat->findUser(action->data(ADR_USER_NICK).toString());
		if (user)
			openPrivateChatWindow(user->userJid());
	}
}

void MultiUserChatWindow::onChangeUserRoleActionTriggeted(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		QString nick = action->data(ADR_USER_NICK).toString();
		QString role = action->data(ADR_USER_ROLE).toString();

		bool ok = true;
		QString reason;
		if (role == MUC_ROLE_NONE)
			reason = QInputDialog::getText(this,tr("Kick User - %1").arg(nick),tr("Enter a message:"),QLineEdit::Normal,QString::null,&ok);

		if (ok)
			FRoleRequestId = FMultiChat->setUserRole(nick,role,reason);
	}
}

void MultiUserChatWindow::onChangeUserAffiliationActionTriggered(bool)
{
	Action *action =qobject_cast<Action *>(sender());
	if (action)
	{
		QString nick = action->data(ADR_USER_NICK).toString();
		QString affiliation = action->data(ADR_USER_AFFIL).toString();

		bool ok = true;
		QString reason;
		if (affiliation == MUC_AFFIL_OUTCAST)
			reason = QInputDialog::getText(this,tr("Ban User - %1").arg(nick),tr("Enter a message:"),QLineEdit::Normal,QString::null,&ok);

		if (ok)
			FAffilRequestId = FMultiChat->setUserAffiliation(nick,affiliation,reason);
	}
}

void MultiUserChatWindow::onStatusIconsChanged()
{
	foreach(IMessageChatWindow *window, FPrivateChatWindows)
		updatePrivateChatWindow(window);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onAutoRejoinAfterKick()
{
	FEnterRoom->trigger();
}

void MultiUserChatWindow::onRoomConfigFormDialogAccepted()
{
	IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
	if (dialog)
		FConfigUpdateRequestId = FMultiChat->updateRoomConfig(dialog->formWidget()->submitDataForm());
}

void MultiUserChatWindow::onRoomConfigFormDialogRejected()
{
	if (FInitializeConfig)
	{
		IDataForm form;
		form.type = DATAFORM_TYPE_SUBMIT;
		FConfigUpdateRequestId = FMultiChat->updateRoomConfig(form);
	}
}

void MultiUserChatWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MUC_GROUPCHAT_USERVIEWMODE)
		FUsersView->setViewMode(ANode.value().toInt());
}

void MultiUserChatWindow::onCentralSplitterHandleMoved(int AOrderId, int ASize)
{
	if (AOrderId == MUCWW_USERSHANDLE)
	{
		if (ASize<=0 && FUsersHide->isChecked())
			FUsersHide->setChecked(false);
		else if (ASize>0 && !FUsersHide->isChecked())
			FUsersHide->setChecked(true);
	}
}

void MultiUserChatWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
		closeTabPage();
}

void MultiUserChatWindow::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHistoryRequests.contains(AId))
	{
		IMessageChatWindow *window = FHistoryRequests.take(AId);
		if (window)
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to load private chat history, room=%1, user=%2, id=%3: %4").arg(contactJid().bare(),window->contactJid().resource(),AId,AError.condition()));
			showPrivateChatStatusMessage(window,tr("Failed to load history: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::StatusError);
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to load multi chat history, room=%1, id=%2: %3").arg(contactJid().bare(),AId,AError.condition()));
			showMultiChatStatusMessage(tr("Failed to load history: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError,true);
		}
		FPendingMessages.remove(window);
		FPendingContent.remove(window);
	}
}

void MultiUserChatWindow::onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody)
{
	if (FHistoryRequests.contains(AId))
	{
		IMessageChatWindow *window = FHistoryRequests.take(AId);

		if (window)
			window->viewWidget()->clearContent();
		else
			FViewWidget->clearContent();

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
					showPrivateChatMessage(window,message);
				else
					showMultiChatUserMessage(message,Jid(message.from()).resource());
				messageIt--;
			}
			else if (noteIt != ABody.notes.constEnd())
			{
				if (window)
					showPrivateChatStatusMessage(window,noteIt.value(),IMessageStyleContentOptions::StatusEmpty,noteIt.key());
				else
					showMultiChatStatusMessage(noteIt.value(),IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusEmpty,true,noteIt.key());
				++noteIt;
			}
		}

		if (!window && !FMultiChat->subject().isEmpty())
			showMultiChatTopic(FMultiChat->subject());

		foreach(const WindowContent &content, FPendingContent.take(window))
		{
			if (window)
			{
				showDateSeparator(window->viewWidget(),content.options.time);
				window->viewWidget()->appendHtml(content.html,content.options);
			}
			else
			{
				showDateSeparator(FViewWidget,content.options.time);
				FViewWidget->appendHtml(content.html,content.options);
			}
		}

		WindowStatus &wstatus = FWindowStatus[window!=NULL ? window->viewWidget() : FViewWidget];
		wstatus.startTime = !ABody.messages.isEmpty() ? ABody.messages.last().dateTime() : QDateTime();

		if (window)
			LOG_STRM_INFO(streamJid(),QString("Private chat history loaded and shown, room=%1, user=%2, id=%3").arg(contactJid().bare(),window->contactJid().resource()).arg(AId));
		else
			LOG_STRM_INFO(streamJid(),QString("Multi chat history loaded and shown, room=%1, id=%2").arg(contactJid().bare(),AId));
	}
}

void MultiUserChatWindow::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IMessageChatWindow *window, FPrivateChatWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setPrivateChatMessageStyle(window);
				requestPrivateChatHistory(window);
			}
		}
	}
	else if (AMessageType==Message::GroupChat && AContext.isEmpty())
	{
		IMessageStyle *style = FViewWidget!=NULL ? FViewWidget->messageStyle() : NULL;
		if (style==NULL || !style->changeOptions(FViewWidget->styleWidget(),AOptions,false))
		{
			setMultiChatMessageStyle();
			requestMultiChatHistory();
		}
	}
}
