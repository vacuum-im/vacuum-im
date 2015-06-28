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
	FLastAffiliation = MUC_AFFIL_NONE;

	FStateLoaded = false;
	FShownDetached = false;
	FDestroyOnChatClosed = false;

	FStartCompletePos = 0;
	FCompleteIt = FCompleteNicks.constEnd();

	FMultiChatManager = AMultiChatManager;

	FMultiChat = AMultiChat;
	FMultiChat->instance()->setParent(this);
	connect(FMultiChat->instance(),SIGNAL(chatAboutToConnect()),SLOT(onChatAboutToConnect()));
	connect(FMultiChat->instance(),SIGNAL(chatOpened()),SLOT(onChatOpened()));
	connect(FMultiChat->instance(),SIGNAL(chatNotify(const QString &)),SLOT(onChatNotify(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(chatError(const QString &)),SLOT(onChatError(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(chatClosed()),SLOT(onChatClosed()));
	connect(FMultiChat->instance(),SIGNAL(roomNameChanged(const QString &)),SLOT(onRoomNameChanged(const QString &)));
	connect(FMultiChat->instance(),SIGNAL(presenceChanged(const IPresenceItem &)),SLOT(onPresenceChanged(const IPresenceItem &)));
	connect(FMultiChat->instance(),SIGNAL(subjectChanged(const QString &, const QString &)),SLOT(onSubjectChanged(const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(serviceMessageReceived(const Message &)),SLOT(onServiceMessageReceived(const Message &)));
	connect(FMultiChat->instance(),SIGNAL(inviteDeclined(const Jid &, const QString &)),SLOT(onInviteDeclined(const Jid &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userChanged(IMultiUser *, int, const QVariant &)),SLOT(onUserChanged(IMultiUser *, int, const QVariant &)));
	connect(FMultiChat->instance(),SIGNAL(userKicked(const QString &, const QString &, const QString &)),SLOT(onUserKicked(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(userBanned(const QString &, const QString &, const QString &)),SLOT(onUserBanned(const QString &, const QString &, const QString &)));
	connect(FMultiChat->instance(),SIGNAL(affiliationListReceived(const QString &,const QList<IMultiUserListItem> &)),SLOT(onAffiliationListReceived(const QString &,const QList<IMultiUserListItem> &)));
	connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)), SLOT(onConfigFormReceived(const IDataForm &)));
	connect(FMultiChat->instance(),SIGNAL(roomDestroyed(const QString &)), SLOT(onRoomDestroyed(const QString &)));

	FMainSplitter = new SplitterWidget(this,Qt::Vertical);
	FMainSplitter->setSpacing(3);
	setCentralWidget(FMainSplitter);

	FCentralSplitter = new SplitterWidget(FMainSplitter,Qt::Horizontal);
	FMainSplitter->insertWidget(MUCWW_CENTRALSPLITTER,FCentralSplitter,100);

	FViewSplitter = new SplitterWidget(FCentralSplitter,Qt::Vertical);
	FCentralSplitter->insertWidget(MUCWW_VIEWSPLITTER,FViewSplitter,75);

	FUsersSplitter = new SplitterWidget(FCentralSplitter,Qt::Vertical);
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
}

MultiUserChatWindow::~MultiUserChatWindow()
{
	QList<IMessageChatWindow *> chatWindows = FPrivateChatWindows;
	foreach(IMessageChatWindow *window,chatWindows)
		delete window->instance();

	if (FMessageProcessor)
	{
		FMessageProcessor->removeMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
	}

	if (FMessageWidgets)
	{
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
	return FTabPageToolTip;
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
	if (AHandlerId==FSHIAnyStanza && FMultiChat->roomJid().pBare()==Jid(AStanza.from()).pBare())
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
		if (AWidget->messageWindow() == this)
			return execShortcutCommand(AWidget->textEdit()->toPlainText());
	}
	else if (AOrder == MESHO_MULTIUSERCHATWINDOW_GROUPCHAT)
	{
		if (AWidget->messageWindow()==this && FMultiChat->isOpen())
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
	if (AMessage.type() != Message::Error)
	{
		if (ADirection == IMessageProcessor::DirectionIn)
		{
			Jid userJid = AMessage.from();
			if (userJid.resource().isEmpty() && !AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
			{
				displayed = true;
				IDataForm form = FDataForms->dataForm(AMessage.stanza().firstElement("x",NS_JABBER_DATA));
				IDataDialogWidget *dialog = FDataForms->dialogWidget(form,this);
				connect(dialog->instance(),SIGNAL(accepted()),SLOT(onDataFormMessageDialogAccepted()));
				showMultiChatStatusMessage(tr("Data form received: %1").arg(form.title),IMessageStyleContentOptions::TypeNotification);
				FDataFormMessages.insert(AMessage.data(MDR_MESSAGE_ID).toInt(),dialog);
				LOG_STRM_INFO(streamJid(),QString("Data form message received, room=%1, from=%2, title=%3").arg(contactJid().bare(),userJid.full(),form.title));
			}
			else if (AMessage.type()==Message::GroupChat || userJid.resource().isEmpty())
			{
				if (!AMessage.body().isEmpty())
				{
					displayed = true;
					if (!AMessage.isDelayed())
						updateRecentItemActiveTime(NULL);
					if (FHistoryRequests.values().contains(NULL))
						FPendingMessages[NULL].append(AMessage);
					showMultiChatUserMessage(AMessage,userJid.resource());
				}
				else
				{
					LOG_STRM_WARNING(streamJid(),QString("Received empty groupchat message, room=%1, from=%2").arg(contactJid().bare(),userJid.full()));
				}
			}
			else if (!AMessage.body().isEmpty())
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
					LOG_STRM_WARNING(streamJid(),QString("Failed to show private chat message, room=%1, user=%2: Private chat window not created").arg(contactJid().bare(),userJid.resource()));
				}
			}
			else
			{
				LOG_STRM_WARNING(streamJid(),QString("Received empty chat message, room=%1, user=%2").arg(contactJid().bare(),userJid.resource()));
			}
		}
		else if (ADirection == IMessageProcessor::DirectionOut)
		{
			Jid userJid = AMessage.to();
			if (!userJid.resource().isEmpty() && !AMessage.body().isEmpty())
			{
				IMessageChatWindow *window = getPrivateChatWindow(userJid);
				if (window)
				{
					displayed = true;
					if (FHistoryRequests.values().contains(window))
						FPendingMessages[window].append(AMessage);
					showPrivateChatMessage(window,AMessage);
				}
				else
				{
					LOG_STRM_WARNING(streamJid(),QString("Failed to show private chat message, room=%1, user=%2: Private chat window not created").arg(contactJid().bare(),userJid.resource()));
				}
			}
		}
	}
	else
	{
		LOG_STRM_WARNING(streamJid(),QString("Error message received, room=%1, from=%2: %3").arg(contactJid().bare(),AMessage.from(),AMessage.body()));
	}
	return displayed;
}

INotification MultiUserChatWindow::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection==IMessageProcessor::DirectionIn && AMessage.type()!=Message::Error)
	{
		Jid userJid = AMessage.from();
		int messageId = AMessage.data(MDR_MESSAGE_ID).toInt();
		IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
		if (!userJid.resource().isEmpty())
		{
			IMessageTabPage *page = NULL;
			if (AMessage.type() == Message::GroupChat)
			{
				if (!AMessage.body().isEmpty() && !AMessage.isDelayed())
				{
					page = this;
					if (isMentionMessage(AMessage))
					{
						notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_MENTION);
						notify.typeId = NNT_MUC_MESSAGE_MENTION;
						notify.data.insert(NDR_TOOLTIP,tr("Mention message in conference: %1").arg(userJid.uNode()));
						notify.data.insert(NDR_POPUP_CAPTION,tr("Mention in conference"));
					}
					else
					{
						notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_GROUPCHAT);
						notify.typeId = NNT_MUC_MESSAGE_GROUPCHAT;
						notify.data.insert(NDR_TOOLTIP,tr("New message in conference: %1").arg(userJid.uNode()));
						notify.data.insert(NDR_POPUP_CAPTION,tr("Conference message"));
					}
					notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_MESSAGE));
					notify.data.insert(NDR_STREAM_JID,streamJid().full());
					notify.data.insert(NDR_CONTACT_JID,userJid.bare());
					notify.data.insert(NDR_ROSTER_ORDER,RNO_GROUPCHATMESSAGE);
					notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
					notify.data.insert(NDR_POPUP_TITLE,tr("[%1] in [%2]").arg(userJid.resource()).arg(userJid.uNode()));
					notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);

					if (!isActiveTabPage())
						FActiveMessages.append(messageId);
					else
						notify.kinds &= Options::node(OPV_NOTIFICATIONS_FORCESOUND).value().toBool() ? INotification::SoundPlay : 0;
				}
			}
			else if (!AMessage.body().isEmpty())
			{
				IMessageChatWindow *window = getPrivateChatWindow(userJid);
				if (window)
				{
					page = window;
					notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_PRIVATE);
					notify.typeId = NNT_MUC_MESSAGE_PRIVATE;
					notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_PRIVATE_MESSAGE));
					notify.data.insert(NDR_TOOLTIP,tr("Private message from: [%1]").arg(userJid.resource()));
					notify.data.insert(NDR_STREAM_JID,streamJid().full());
					notify.data.insert(NDR_CONTACT_JID,userJid.full());
					notify.data.insert(NDR_ROSTER_ORDER,RNO_GROUPCHATMESSAGE);
					notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
					notify.data.insert(NDR_POPUP_CAPTION,tr("Private message"));
					notify.data.insert(NDR_POPUP_TITLE,QString("[%1]").arg(userJid.resource()));
					notify.data.insert(NDR_SOUND_FILE,SDF_MUC_PRIVATE_MESSAGE);

					if (!window->isActiveTabPage())
					{
						if (FDestroyTimers.contains(window))
							delete FDestroyTimers.take(window);
						FActiveChatMessages.insertMulti(window,messageId);

						IMultiUserViewNotify notify;
						notify.flags |= IMultiUserViewNotify::Blink;
						notify.icon = storage->getIcon(MNI_MUC_PRIVATE_MESSAGE);

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
			if (notify.kinds & INotification::RosterNotify)
			{
				QMap<QString,QVariant> searchData;
				searchData.insert(QString::number(RDR_STREAM_JID),streamJid().pFull());

				if (FRecentContacts!=NULL && AMessage.type()==Message::Chat && !userJid.resource().isEmpty())
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
		else if (!AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
		{
			IDataDialogWidget *dialog = FDataFormMessages.value(messageId);
			if (dialog && !dialog->instance()->isActiveWindow())
			{
				notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_PRIVATE);
				notify.typeId = NNT_MUC_MESSAGE_PRIVATE;
				notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_DATA_MESSAGE));
				notify.data.insert(NDR_TOOLTIP,tr("Data form received from: %1").arg(userJid.uNode()));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Data form received"));
				notify.data.insert(NDR_POPUP_TITLE,ANotifications->contactName(FMultiChat->streamJid(),userJid));
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(userJid));
				notify.data.insert(NDR_POPUP_TEXT,AMessage.stanza().firstElement("x",NS_JABBER_DATA).firstChildElement("instructions").text());
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
		IMessageChatWindow *window = FActiveChatMessages.key(AMessageId);
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
	else
	{
		REPORT_ERROR("Failed to show notified groupchat message window: Window not found");
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
				LOG_STRM_WARNING(AStreamJid,QString("Failed to show private chat window, room=%1, user=%2: Private chat window not created").arg(contactJid().bare(),AContactJid.resource()));
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
		AMenu->addAction(FBanList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FMembersList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FAdminsList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FOwnersList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FConfigRoom,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FDestroyRoom,AG_RVCM_MULTIUSERCHAT_TOOLS);
	}
	else if (affiliation == MUC_AFFIL_ADMIN)
	{
		AMenu->addAction(FChangeNick,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FChangeTopic,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FInviteContact,AG_RVCM_MULTIUSERCHAT_COMMON);
		AMenu->addAction(FBanList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FMembersList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FAdminsList,AG_RVCM_MULTIUSERCHAT_TOOLS);
		AMenu->addAction(FOwnersList,AG_RVCM_MULTIUSERCHAT_TOOLS);
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
				setRoleNone->setText(tr("Kick User"));
				setRoleNone->setData(ADR_USER_NICK,AUser->nick());
				setRoleNone->setData(ADR_USER_ROLE,MUC_ROLE_NONE);
				connect(setRoleNone,SIGNAL(triggered(bool)),SLOT(onChangeUserRoleActionTriggeted(bool)));
				AMenu->addAction(setRoleNone,AG_MUCM_MULTIUSERCHAT_UTILS,false);

				Action *setAffilOutcast = new Action(AMenu);
				setAffilOutcast->setText(tr("Ban User"));
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

	bool waitClose = false;
	FDestroyOnChatClosed = true;

	if (FMultiChat->isConnected())
	{
		waitClose = true;
		FMultiChat->sendPresence(IPresence::Offline,AStatus,0);
	}

	if (AWaitClose > 0)
		QTimer::singleShot(waitClose ? AWaitClose : 0, this, SLOT(deleteLater()));
	else
		delete this;
}

void MultiUserChatWindow::initialize()
{
	FMessageWidgets = PluginHelper::pluginInstance<IMessageWidgets>();
	if (FMessageWidgets)
	{
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

		FViewWidget = FMessageWidgets->newViewWidget(this,FCentralSplitter);
		connect(FViewWidget->instance(),SIGNAL(contentAppended(const QString &, const IMessageStyleContentOptions &)),
			SLOT(onMultiChatContentAppended(const QString &, const IMessageStyleContentOptions &)));
		connect(FViewWidget->instance(),SIGNAL(messageStyleOptionsChanged(const IMessageStyleOptions &, bool)),
			SLOT(onMultiChatMessageStyleOptionsChanged(const IMessageStyleOptions &, bool)));
		FViewSplitter->insertWidget(MUCWW_VIEWWIDGET,FViewWidget->instance(),100);
		FWindowStatus[FViewWidget].createTime = QDateTime::currentDateTime();

		FUsersView = new MultiUserView(FMultiChat,FCentralSplitter);
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

	FBanList = new Action(this);
	FBanList->setText(tr("Edit Ban List"));
	FBanList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_BAN_LIST);
	connect(FBanList,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FMembersList = new Action(this);
	FMembersList->setText(tr("Edit Members List"));
	FMembersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_MEMBERS_LIST);
	connect(FMembersList,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FAdminsList = new Action(this);
	FAdminsList->setText(tr("Edit Administrators List"));
	FAdminsList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_ADMINS_LIST);
	connect(FAdminsList,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FOwnersList = new Action(this);
	FOwnersList->setText(tr("Edit Owners List"));
	FOwnersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_OWNERS_LIST);
	connect(FOwnersList,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FConfigRoom = new Action(this);
	FConfigRoom->setText(tr("Configure Conference"));
	FConfigRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFIGURE_ROOM);
	connect(FConfigRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FDestroyRoom = new Action(this);
	FDestroyRoom->setText(tr("Destroy Conference"));
	FDestroyRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_DESTROY_ROOM);
	connect(FDestroyRoom,SIGNAL(triggered(bool)),SLOT(onRoomActionTriggered(bool)));

	FClearChat = new Action(this);
	FClearChat->setText(tr("Clear Conference Window"));
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
}

void MultiUserChatWindow::saveWindowState()
{
	if (FStateLoaded)
	{
		int size = FCentralSplitter->handleSize(MUCWW_USERSHANDLE);
		Options::setFileValue(size>0 ? size : -1,"muc.mucwindow.users-list-width",tabPageId());
	}
}

void MultiUserChatWindow::loadWindowState()
{
	int size = Options::fileValue("muc.mucwindow.users-list-width",tabPageId()).toInt();
	if (size > 0)
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,size);
	else if (size < 0)
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,0);
	else
		FCentralSplitter->setHandleSize(MUCWW_USERSHANDLE,130);
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
	if (AText.startsWith("/kick "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->findUser(nick))
			FMultiChat->setRole(nick,MUC_ROLE_NONE,parts.join(" "));
		else
			showMultiChatStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/ban "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->findUser(nick))
			FMultiChat->setAffiliation(nick,MUC_AFFIL_OUTCAST,parts.join(" "));
		else
			showMultiChatStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
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
			showMultiChatStatusMessage(tr("%1 is not valid contact JID").arg(userJid.uFull()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/join "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		Jid joinRoomJid = Jid::fromUserInput(parts.takeFirst() + "@" + FMultiChat->roomJid().domain());
		if (joinRoomJid.isValid())
			FMultiChatManager->showJoinMultiChatDialog(streamJid(),joinRoomJid,FMultiChat->nickName(),parts.join(" "));
		else
			showMultiChatStatusMessage(tr("%1 is not valid room JID").arg(joinRoomJid.uBare()),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		hasCommand = true;
	}
	else if (AText.startsWith("/msg "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.takeFirst();
		if (FMultiChat->findUser(nick))
		{
			Message message;
			message.setBody(parts.join(" "));
			FMultiChat->sendMessage(message,nick);
		}
		else
		{
			showMultiChatStatusMessage(tr("User %1 is not present in the conference").arg(nick),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
		}
		hasCommand = true;
	}
	else if (AText.startsWith("/nick "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString nick = parts.join(" ");
		FMultiChat->setNickName(nick);
		hasCommand = true;
	}
	else if (AText.startsWith("/part ") || AText.startsWith("/leave ") || AText=="/part" || AText=="/leave")
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString status = parts.join(" ");
		FMultiChat->sendPresence(IPresence::Offline,status,0);
		exitAndDestroy(QString::null);
		hasCommand = true;
	}
	else if (AText.startsWith("/topic "))
	{
		QStringList parts = AText.split(" ");
		parts.removeFirst();
		QString subject = parts.join(" ");
		FMultiChat->sendSubject(subject);
		hasCommand = true;
	}
	else if (AText == "/help")
	{
		showMultiChatStatusMessage(tr("Supported list of commands: \n"
			" /ban <roomnick> [comment] \n"
			" /invite <jid> [comment] \n"
			" /join <roomname> [pass] \n"
			" /kick <roomnick> [comment] \n"
			" /msg <roomnick> <foo> \n"
			" /nick <newnick> \n"
			" /leave [comment] \n"
			" /topic <foo>"),IMessageStyleContentOptions::TypeNotification);
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

bool MultiUserChatWindow::isMentionMessage(const Message &AMessage) const
{
	QString message = AMessage.body();
	QString nick = FMultiChat->nickName();

	// QString::indexOf will not work if nick ends with '+'
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
	QString topic = ANick.isEmpty() ? tr("Subject: %1").arg(ATopic) : tr("%1 has changed the subject to: %2").arg(ANick).arg(ATopic);

	IMessageStyleContentOptions options;
	options.kind = IMessageStyleContentOptions::KindTopic;
	options.type |= IMessageStyleContentOptions::TypeGroupchat;
	options.direction = IMessageStyleContentOptions::DirectionIn;

	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyleManager->timeFormat(options.time);

	showDateSeparator(FViewWidget,options.time);
	FViewWidget->appendText(topic,options);
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
			statuses.append(qMakePair<QString,int>(tr("This room is non-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_AFFIL_CHANGED))
			statuses.append(qMakePair<QString,int>(tr("%1 affiliation changed while not in the room").arg(ANick),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_MEMBERS_SHOW))
			statuses.append(qMakePair<QString,int>(tr("Room now shows unavailable members"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_MEMBERS_HIDE))
			statuses.append(qMakePair<QString,int>(tr("Room now does not show unavailable members"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_CONFIG_CHANGED))
			statuses.append(qMakePair<QString,int>(tr("Room configuration change has occurred"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_LOGGING_ENABLED))
			statuses.append(qMakePair<QString,int>(tr("Room logging is now enabled"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_LOGGING_DISABLED))
			statuses.append(qMakePair<QString,int>(tr("Room logging is now disabled"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_NON_ANONYMOUS))
			statuses.append(qMakePair<QString,int>(tr("The room is now non-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_NOW_SEMI_ANONYMOUS))
			statuses.append(qMakePair<QString,int>(tr("The room is now semi-anonymous"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_ROOM_CREATED))
			statuses.append(qMakePair<QString,int>(tr("A new room has been created"),IMessageStyleContentOptions::TypeNotification));

		if (ACodes.contains(MUC_SC_AFFIL_CHANGE))
			statuses.append(qMakePair<QString,int>(tr("%1 has been removed from the room because of an affiliation change").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		if (ACodes.contains(MUC_SC_MEMBERS_ONLY))
			statuses.append(qMakePair<QString,int>(tr("%1 has been removed from the room because the room has been changed to members-only").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		if (ACodes.contains(MUC_SC_SYSTEM_SHUTDOWN))
			statuses.append(qMakePair<QString,int>(tr("%1 is being removed from the room because of a system shutdown").arg(ANick),IMessageStyleContentOptions::TypeEvent));

		if (ACodes.contains(MUC_SC_SELF_PRESENCE))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		if (ACodes.contains(MUC_SC_NICK_CHANGED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		if (ACodes.contains(MUC_SC_USER_BANNED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		if (ACodes.contains(MUC_SC_NICK_ASSIGNED))
			statuses.append(qMakePair<QString,int>(QString::null,0));

		if (ACodes.contains(MUC_SC_USER_KICKED))
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
		options.senderIcon = FMessageStyleManager->contactIcon(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false);
	else
		options.senderIcon = FMessageStyleManager->contactIcon(Jid::null,IPresence::Offline,SUBSCRIPTION_BOTH,false);

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
	infoWidget()->setFieldValue(IMessageInfoWidget::Name,FMultiChat->roomName());

	QIcon statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(contactJid(),FMultiChat->roomPresence().show,SUBSCRIPTION_BOTH,false) : QIcon();
	infoWidget()->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);
	infoWidget()->setFieldValue(IMessageInfoWidget::StatusText,FMultiChat->subject());

	QIcon tabIcon = statusIcon;
	if (tabPageNotifier() && tabPageNotifier()->activeNotify()>0)
		tabIcon = tabPageNotifier()->notifyById(tabPageNotifier()->activeNotify()).icon;

	setWindowIcon(tabIcon);
	setWindowIconText(QString("%1 (%2)").arg(FMultiChat->roomShortName()).arg(FUsers.count()));
	setWindowTitle(tr("%1 - Conference").arg(FMultiChat->roomName()));

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
				clearAction->setText(tr("Clear Chat Window"));
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
		AOptions.senderIcon = FMessageStyleManager->contactIcon(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false);

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

		QString name = tr("[%1] in [%2]").arg(user->nick(),FMultiChat->roomShortName());
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::Name,name);

		QIcon statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(user->userJid(),user->presence().show,SUBSCRIPTION_BOTH,false) : QIcon();
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusIcon,statusIcon);
		AWindow->infoWidget()->setFieldValue(IMessageInfoWidget::StatusText,user->presence().status);

		QIcon tabIcon = statusIcon;
		if (AWindow->tabPageNotifier() && AWindow->tabPageNotifier()->activeNotify()>0)
			tabIcon = AWindow->tabPageNotifier()->notifyById(AWindow->tabPageNotifier()->activeNotify()).icon;

		AWindow->updateWindow(tabIcon,name,tr("%1 - Private chat").arg(name),QString::null);
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

void MultiUserChatWindow::onChatAboutToConnect()
{
	IMultiUserChatHistory history;
	if (FLastStanzaTime.isValid())
	{
		if (FLastAffiliation != MUC_AFFIL_NONE)
			history.seconds = FLastStanzaTime.secsTo(QDateTime::currentDateTime());
		else
			history.since = FLastStanzaTime; // Time lost is possible if CAPTCHA enabled
	}
	FMultiChat->setHistory(history);
}

void MultiUserChatWindow::onChatOpened()
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

	if (FMultiChat->statusCodes().contains(MUC_SC_ROOM_CREATED))
		FMultiChat->requestConfigForm();
	showMultiChatStatusMessage(tr("You entered into the room"),IMessageStyleContentOptions::TypeEvent,IMessageStyleContentOptions::StatusOnline);
}

void MultiUserChatWindow::onChatNotify(const QString &ANotify)
{
	showMultiChatStatusMessage(tr("Notify: %1").arg(ANotify),IMessageStyleContentOptions::TypeNotification);
}

void MultiUserChatWindow::onChatError(const QString &AMessage)
{
	showMultiChatStatusMessage(tr("Error: %1").arg(AMessage),IMessageStyleContentOptions::TypeNotification,IMessageStyleContentOptions::StatusError);
}

void MultiUserChatWindow::onChatClosed()
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIAnyStanza);
		FSHIAnyStanza = -1;
	}

	if (!FDestroyOnChatClosed)
	{
		bool isConflictError = FMultiChat->roomError().toStanzaError().conditionCode()==XmppStanzaError::EC_CONFLICT;
		if (FMultiChat->roomPresence().show==IPresence::Error && isConflictError && !FMultiChat->nickName().endsWith("/"+FMultiChat->streamJid().resource()))
		{
			LOG_STRM_INFO(streamJid(),QString("Adding resource to nick due it has been registered by some one else, room=%1, nick=%2").arg(contactJid().bare(),FMultiChat->nickName()));
			FMultiChat->setNickName(FMultiChat->nickName()+"/"+FMultiChat->streamJid().resource());
			FEnterRoom->trigger();
		}
		else
		{
			showMultiChatStatusMessage(tr("You left the room"),IMessageStyleContentOptions::TypeEvent,IMessageStyleContentOptions::StatusOffline);
		}
		updateMultiChatWindow();
	}
	else
	{
		deleteLater();
	}
}

void MultiUserChatWindow::onRoomNameChanged(const QString &AName)
{
	Q_UNUSED(AName);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onPresenceChanged(const IPresenceItem &APresence)
{
	Q_UNUSED(APresence);
	QAction *enterHandler = FToolBarWidget->toolBarChanger()->actionHandle(FEnterRoom);
	if (enterHandler)
		enterHandler->setVisible(!FMultiChat->isOpen());
	updateMultiChatWindow();
}

void MultiUserChatWindow::onSubjectChanged(const QString &ANick, const QString &ASubject)
{
	showMultiChatTopic(ASubject,ANick);
	updateMultiChatWindow();
}

void MultiUserChatWindow::onServiceMessageReceived(const Message &AMessage)
{
	if (!showMultiChatStatusCodes(FMultiChat->statusCodes(),QString::null,AMessage.body()))
		messageDisplay(AMessage,IMessageProcessor::DirectionIn);
}

void MultiUserChatWindow::onInviteDeclined(const Jid &AContactJid, const QString &AReason)
{
	QString nick = AContactJid && contactJid() ? AContactJid.resource() : AContactJid.uFull();
	showMultiChatStatusMessage(tr("%1 has declined your invite to this room. %2").arg(nick).arg(AReason),IMessageStyleContentOptions::TypeNotification);
}

void MultiUserChatWindow::onUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore)
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
						enterMessage = tr("%1 <%2> has joined the room").arg(AUser->nick()).arg(AUser->realJid().uFull());
					else
						enterMessage = tr("%1 has joined the room").arg(AUser->nick());
					if (!presence.status.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
						enterMessage += QString(" - [%1] %2").arg(show).arg(presence.status);
					showMultiChatStatusMessage(enterMessage,IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusJoined);
				}

				refreshCompleteNicks();
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
						enterMessage = tr("%1 <%2> has left the room").arg(AUser->nick()).arg(AUser->realJid().uBare());
					else
						enterMessage = tr("%1 has left the room").arg(AUser->nick());
					if (!presence.status.isEmpty() && Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS).value().toBool())
						enterMessage += " - " + presence.status.trimmed();
					showMultiChatStatusMessage(enterMessage,IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusLeft);
				}
			}

			FUsers.remove(AUser);
			refreshCompleteNicks();
			updateMultiChatWindow();
		}

		if (AUser == FMultiChat->mainUser())
			FLastAffiliation = AUser->affiliation();

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
			showMultiChatStatusMessage(tr("%1 role changed from %2 to %3").arg(AUser->nick()).arg(ABefore.toString()).arg(AUser->role()),IMessageStyleContentOptions::TypeEvent);
	}
	else if (AData == MUDR_AFFILIATION)
	{
		if (FUsers.contains(AUser))
			showMultiChatStatusMessage(tr("%1 affiliation changed from %2 to %3").arg(AUser->nick()).arg(ABefore.toString()).arg(AUser->affiliation()),IMessageStyleContentOptions::TypeEvent);
	}
}

void MultiUserChatWindow::onUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->findUser(ANick);
	Jid realJid = user!=NULL ? user->realJid() : Jid::null;

	showMultiChatStatusMessage(tr("%1 has been kicked from the room%2. %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uBare()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageStyleContentOptions::TypeEvent);

	if (Options::node(OPV_MUC_GROUPCHAT_REJOINAFTERKICK).value().toBool() && user==FMultiChat->mainUser())
		QTimer::singleShot(REJOIN_AFTER_KICK_MSEC,this,SLOT(onAutoRejoinAfterKick()));
}

void MultiUserChatWindow::onUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser)
{
	IMultiUser *user = FMultiChat->findUser(ANick);
	Jid realJid = user!=NULL ? user->realJid() : Jid::null;

	showMultiChatStatusMessage(tr("%1 has been banned from the room%2. %3")
		.arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid.uFull()) : ANick)
		.arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
		.arg(AReason), IMessageStyleContentOptions::TypeEvent);
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
	dialog->setTitle(listName.arg(contactJid().uBare()));
	connect(dialog,SIGNAL(accepted()),SLOT(onAffiliationListDialogAccepted()));
	connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog,SLOT(reject()));
	dialog->show();
}

void MultiUserChatWindow::onConfigFormReceived(const IDataForm &AForm)
{
	if (FDataForms)
	{
		IDataForm localizedForm(FDataForms->localizeForm(AForm));
		localizedForm.title = QString("%1 (%2)").arg(localizedForm.title, FMultiChat->roomJid().uBare());
		IDataDialogWidget *dialog = FDataForms->dialogWidget(localizedForm,this);
		connect(dialog->instance(),SIGNAL(accepted()),SLOT(onConfigFormDialogAccepted()));
		connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog->instance(),SLOT(reject()));
		connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)),dialog->instance(),SLOT(reject()));
		dialog->instance()->show();
	}
}

void MultiUserChatWindow::onRoomDestroyed(const QString &AReason)
{
	showMultiChatStatusMessage(tr("This room was destroyed by owner. %1").arg(AReason),IMessageStyleContentOptions::TypeEvent);
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
		QString nick = QInputDialog::getText(this,tr("Change nick name"),tr("Enter your new nick name in conference %1").arg(contactJid().uNode()),
			QLineEdit::Normal,FMultiChat->nickName());
		if (!nick.isEmpty())
			FMultiChat->setNickName(nick);
	}
	else if (action == FChangeTopic)
	{
		if (FMultiChat->isOpen())
		{
			QString newSubject = FMultiChat->subject();
			InputTextDialog *dialog = new InputTextDialog(this,tr("Change topic"),tr("Enter new topic for conference %1").arg(contactJid().uNode()), newSubject);
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
			Jid contactJid = QInputDialog::getText(this,tr("Invite user"),tr("Enter user JID:"));
			if (contactJid.isValid())
			{
				QString reason = tr("Please, enter this conference!");
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
			QString reason = QInputDialog::getText(this,tr("Destroying conference"),tr("Enter a reason:"),QLineEdit::Normal,QString::null,&ok);
			if (ok)
				FMultiChat->destroyRoom(reason);
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

void MultiUserChatWindow::onStatusIconsChanged()
{
	foreach(IMessageChatWindow *window, FPrivateChatWindows) {
		updatePrivateChatWindow(window);	}
	updateMultiChatWindow();
}

void MultiUserChatWindow::onAutoRejoinAfterKick()
{
	FMultiChat->sendStreamPresence();
}

void MultiUserChatWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MUC_GROUPCHAT_USERVIEWMODE)
	{
		FUsersView->setViewMode(ANode.value().toInt());
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
			showPrivateChatStatusMessage(window,tr("Failed to load history: %1").arg(AError.errorMessage()));
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to load multi chat history, room=%1, id=%2: %3").arg(contactJid().bare(),AId,AError.condition()));
			showMultiChatStatusMessage(tr("Failed to load history: %1").arg(AError.errorMessage()),IMessageStyleContentOptions::TypeEmpty,IMessageStyleContentOptions::StatusEmpty,true);
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
