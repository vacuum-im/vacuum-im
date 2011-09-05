#include "normalmessagehandler.h"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_GROUP                 Action::DR_Parametr3

static const QList<int> MessageActionTypes = QList<int>() << RIT_STREAM_ROOT << RIT_GROUP << RIT_GROUP_BLANK
  << RIT_GROUP_AGENTS << RIT_GROUP_MY_RESOURCES << RIT_GROUP_NOT_IN_ROSTER << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

NormalMessageHandler::NormalMessageHandler()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FStatusIcons = NULL;
	FPresencePlugin = NULL;
	FRostersView = NULL;
	FXmppUriQueries = NULL;
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
			connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
				SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
		if (notifications)
		{
			INotificationType notifyType;
			notifyType.order = NTO_NORMALHANDLER_MESSAGE;
			notifyType.title = tr("When receiving new single message");
			notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
			notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
			notifications->registerNotificationType(NNT_NORMAL_MESSAGE,notifyType);
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRosterIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool NormalMessageHandler::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWNORMALDIALOG, tr("Send message"), tr("Ctrl+Return","Send message"), Shortcuts::WidgetShortcut);

	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(this,MHO_NORMALMESSAGEHANDLER);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this,XUHO_DEFAULT);
	}
	if (FRostersView)
	{
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWNORMALDIALOG, FRostersView->instance());
	}
	return true;
}

bool NormalMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type.isEmpty() || type=="normal")
		{
			IMessageWindow *window = getWindow(AStreamJid, AContactJid, IMessageWindow::WriteMode);
			if (AParams.contains("thread"))
				window->setThreadId(AParams.value("thread"));
			window->setSubject(AParams.value("subject"));
			window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
			window->showTabPage();
			return true;
		}
	}
	return false;
}

bool NormalMessageHandler::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	if (!AMessage.body().isEmpty() || !AMessage.subject().isEmpty())
		return true;
	return false;
}

bool NormalMessageHandler::showMessage(int AMessageId)
{
	IMessageWindow *window = FActiveMessages.key(AMessageId);
	if (!window)
	{
		Message message = FMessageProcessor->messageById(AMessageId);
		return createMessageWindow(MHO_NORMALMESSAGEHANDLER,message.to(),message.from(),Message::Normal,IMessageHandler::SM_SHOW);
	}
	else
	{
		window->showTabPage();
		return true;
	}
	return false;
}

bool NormalMessageHandler::receiveMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	IMessageWindow *window = findWindow(message.to(),message.from());
	if (window)
	{
		FActiveMessages.insertMulti(window,AMessageId);
		updateWindow(window);
	}
	else
	{
		FActiveMessages.insertMulti(NULL,AMessageId);
	}
	return true;
}

INotification NormalMessageHandler::notifyMessage(INotifications *ANotifications, const Message &AMessage)
{
	IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	QIcon icon =  storage->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
	QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

	INotification notify;
	notify.kinds = ANotifications->notificationKinds(NNT_NORMAL_MESSAGE);
	if (notify.kinds > 0)
	{
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

		IMessageWindow *window = FActiveMessages.key(AMessage.data(MDR_MESSAGE_ID).toInt());
		if (window)
		{
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)window->instance());
			notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
			notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_NEW_MESSAGE);
			notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)window->instance());
		}

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

	return notify;
}

bool NormalMessageHandler::createMessageWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AType);
	IMessageWindow *window = getWindow(AStreamJid,AContactJid,IMessageWindow::WriteMode);
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
	return false;
}

IMessageWindow *NormalMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
	IMessageWindow *window = NULL;
	if (AStreamJid.isValid() && (AContactJid.isValid() || AMode == IMessageWindow::WriteMode))
	{
		window = FMessageWidgets->newMessageWindow(AStreamJid,AContactJid,AMode);
		if (window)
		{
			window->infoWidget()->autoUpdateFields();
			connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
			connect(window->instance(),SIGNAL(showNextMessage()),SLOT(onShowNextMessage()));
			connect(window->instance(),SIGNAL(replyMessage()),SLOT(onReplyMessage()));
			connect(window->instance(),SIGNAL(forwardMessage()),SLOT(onForwardMessage()));
			connect(window->instance(),SIGNAL(showChatWindow()),SLOT(onShowChatWindow()));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));
			FWindows.append(window);
			loadActiveMessages(window);
			showNextMessage(window);
		}
		else
			window = findWindow(AStreamJid,AContactJid);
	}
	return window;
}

IMessageWindow *NormalMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	foreach(IMessageWindow *window,FWindows)
		if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
			return window;
	return NULL;
}

void NormalMessageHandler::showNextMessage(IMessageWindow *AWindow)
{
	if (FActiveMessages.contains(AWindow))
	{
		int messageId = FActiveMessages.value(AWindow);
		Message message = FMessageProcessor->messageById(messageId);
		showStyledMessage(AWindow,message);
		FLastMessages.insert(AWindow,message);
		FMessageProcessor->removeMessage(messageId);
		FActiveMessages.remove(AWindow,messageId);
	}
	updateWindow(AWindow);
}

void NormalMessageHandler::loadActiveMessages(IMessageWindow *AWindow)
{
	QList<int> messagesId = FActiveMessages.values(NULL);
	foreach(int messageId, messagesId)
	{
		Message message = FMessageProcessor->messageById(messageId);
		if (AWindow->streamJid() == message.to() && AWindow->contactJid() == message.from())
		{
			FActiveMessages.insertMulti(AWindow,messageId);
			FActiveMessages.remove(NULL,messageId);
		}
	}
}

void NormalMessageHandler::updateWindow(IMessageWindow *AWindow)
{
	QIcon icon;
	if (AWindow->instance()->isWindow() && FActiveMessages.contains(AWindow))
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
	else if (FStatusIcons)
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString caption = tr("Composing message");
	if (AWindow->mode() == IMessageWindow::ReadMode)
		caption = tr("%1 - Message").arg(AWindow->infoWidget()->field(IInfoWidget::ContactName).toString());
	AWindow->updateWindow(icon,caption,caption,QString::null);
	AWindow->setNextCount(FActiveMessages.count(AWindow));
}

void NormalMessageHandler::setMessageStyle(IMessageWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Normal);
	if (AWindow->viewWidget()->messageStyle()==NULL || !AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true))
	{
		IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
		AWindow->viewWidget()->setMessageStyle(style,soptions);
	}
}

void NormalMessageHandler::fillContentOptions(IMessageWindow *AWindow, IMessageContentOptions &AOptions) const
{
	AOptions.senderColor = "blue";
	AOptions.senderId = AWindow->contactJid().full();
	AOptions.senderName = Qt::escape(FMessageStyles->contactName(AWindow->streamJid(),AWindow->contactJid()));
	AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->contactJid());
	AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid(),AWindow->contactJid());
}

void NormalMessageHandler::showStyledMessage(IMessageWindow *AWindow, const Message &AMessage)
{
	IMessageContentOptions options;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	options.noScroll = true;
	fillContentOptions(AWindow,options);

	AWindow->setMode(IMessageWindow::ReadMode);
	AWindow->setSubject(AMessage.subject());
	AWindow->setThreadId(AMessage.threadId());

	setMessageStyle(AWindow);

	if (AMessage.type() == Message::Error)
	{
		ErrorHandler err(AMessage.stanza().element());
		QString html = tr("<b>The message with a error code %1 is received</b>").arg(err.code());
		html += "<p style='color:red;'>"+Qt::escape(err.message())+"</p>";
		html += "<hr>";
		options.kind = IMessageContentOptions::KindMessage;
		AWindow->viewWidget()->appendHtml(html,options);
	}

	options.kind = IMessageContentOptions::KindTopic;
	AWindow->viewWidget()->appendText(tr("Subject: %1").arg(!AMessage.subject().isEmpty() ? AMessage.subject() : tr("<no subject>")),options);
	options.kind = IMessageContentOptions::KindMessage;
	AWindow->viewWidget()->appendMessage(AMessage,options);
}

bool NormalMessageHandler::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> groupTypes = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK << RIT_GROUP_AGENTS 
		<< RIT_GROUP_MY_RESOURCES << RIT_GROUP_NOT_IN_ROSTER;
	static const QList<int> contactTypes =  QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;
	if (!ASelected.isEmpty())
	{
		Jid singleStream;
		bool hasGroups = false;
		bool hasContacts = false;
		foreach(IRosterIndex *index, ASelected)
		{
			int indexType = index->type();
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (!MessageActionTypes.contains(indexType))
				return false;
			else if(!singleStream.isEmpty() && singleStream!=streamJid)
				return false;
			else if (indexType==RIT_STREAM_ROOT && ASelected.count()>1)
				return false;
			else if (hasGroups && !groupTypes.contains(indexType))
				return false;
			else if (hasContacts && !contactTypes.contains(indexType))
				return false;
			singleStream = streamJid;
			hasGroups = hasGroups || groupTypes.contains(indexType);
			hasContacts = hasContacts || contactTypes.contains(indexType);
		}
		return true;
	}
	return false;
}

void NormalMessageHandler::onMessageReady()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (window)
	{
		Message message;
		message.setType(Message::Normal);
		message.setSubject(window->subject());
		message.setThreadId(window->threadId());
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			bool sended = false;
			QList<Jid> receiversList = window->receiversWidget()->receivers();
			foreach(Jid receiver, receiversList)
			{
				message.setTo(receiver.eFull());
				sended = FMessageProcessor->sendMessage(window->streamJid(),message) ? true : sended;
			}
			if (sended)
			{
				if (FActiveMessages.contains(window))
					showNextMessage(window);
				else
					window->closeTabPage();
			}
		}
	}
}

void NormalMessageHandler::onShowNextMessage()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (window)
	{
		showNextMessage(window);
		updateWindow(window);
	}
}

void NormalMessageHandler::onReplyMessage()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (window)
	{
		window->setMode(IMessageWindow::WriteMode);
		window->setSubject(tr("Re: %1").arg(window->subject()));
		window->editWidget()->clearEditor();
		window->editWidget()->textEdit()->setFocus();
		updateWindow(window);
	}
}

void NormalMessageHandler::onForwardMessage()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (FLastMessages.contains(window))
	{
		Message message = FLastMessages.value(window);
		window->setMode(IMessageWindow::WriteMode);
		window->setSubject(tr("Fw: %1").arg(message.subject()));
		window->setThreadId(message.threadId());
		FMessageProcessor->messageToText(window->editWidget()->document(),message);
		window->editWidget()->textEdit()->setFocus();
		window->receiversWidget()->clear();
		window->setCurrentTabWidget(window->receiversWidget()->instance());
		updateWindow(window);
	}
}

void NormalMessageHandler::onShowChatWindow()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (FMessageProcessor && window)
		FMessageProcessor->createMessageWindow(window->streamJid(),window->contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
}

void NormalMessageHandler::onWindowDestroyed()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (FWindows.contains(window))
	{
		QList<int> messagesId = FActiveMessages.values(window);
		foreach(int messageId, messagesId)
			FActiveMessages.insertMulti(NULL,messageId);
		FActiveMessages.remove(window);
		FLastMessages.remove(window);
		FWindows.removeAt(FWindows.indexOf(window));
	}
}

void NormalMessageHandler::onStatusIconsChanged()
{
	foreach(IMessageWindow *window, FWindows)
		updateWindow(window);
}

void NormalMessageHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList contacts = action->data(ADR_CONTACT_JID).toStringList();
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = contacts.count()==1 ? contacts.first() : QString::null;
		if (createMessageWindow(MHO_NORMALMESSAGEHANDLER,streamJid,contactJid,Message::Normal,IMessageHandler::SM_SHOW))
		{
			IMessageWindow *window = FMessageWidgets->findMessageWindow(streamJid,contactJid);
			if (window)
			{
				foreach(QString group, action->data(ADR_GROUP).toStringList())
					window->receiversWidget()->addReceiversGroup(group);

				foreach(QString contactJid, action->data(ADR_CONTACT_JID).toStringList())
					window->receiversWidget()->addReceiver(contactJid);
			}
		}
	}
}

void NormalMessageHandler::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (AId == SCT_ROSTERVIEW_SHOWNORMALDIALOG && isSelectionAccepted(indexes))
		{
			Jid streamJid = indexes.first()->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
			if (presence && presence->isOpen())
			{
				QStringList groups;
				QStringList contacts;
				foreach(IRosterIndex *index, indexes)
				{
					if (index->type() == RIT_GROUP)
						groups.append(index->data(RDR_GROUP).toString());
					else if (index->type()>=RIT_GROUP_BLANK && index->type()<=RIT_GROUP_AGENTS)
						groups.append(FRostersView->rostersModel()->singleGroupName(index->type()));
					else if (index->type() != RIT_STREAM_ROOT)
						contacts.append(index->data(RDR_FULL_JID).toString());
				}

				Jid contactJid = contacts.count()==1 ? contacts.first() : QString::null;
				if (createMessageWindow(MHO_NORMALMESSAGEHANDLER,streamJid,contactJid,Message::Normal,IMessageHandler::SM_SHOW))
				{
					IMessageWindow *window = FMessageWidgets->findMessageWindow(streamJid,contactJid);
					if (window)
					{
						foreach(QString group, groups)
							window->receiversWidget()->addReceiversGroup(group);

						foreach(QString contactJid, contacts)
							window->receiversWidget()->addReceiver(contactJid);
					}
				}
			}
		}
	}
}

void NormalMessageHandler::onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void NormalMessageHandler::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if (ALabelId==RLID_DISPLAY && isSelectionAccepted(AIndexes))
	{
		Jid streamJid = AIndexes.first()->data(RDR_STREAM_JID).toString();
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
		if (presence && presence->isOpen())
		{
			QStringList groups;
			QStringList contacts;
			foreach(IRosterIndex *index, AIndexes)
			{
				if (index->type() == RIT_GROUP)
					groups.append(index->data(RDR_GROUP).toString());
				else if (index->type()>=RIT_GROUP_BLANK && index->type()<=RIT_GROUP_AGENTS)
					groups.append(FRostersView->rostersModel()->singleGroupName(index->type()));
				else if (index->type() != RIT_STREAM_ROOT)
					contacts.append(index->data(RDR_FULL_JID).toString());
			}

			Action *action = new Action(AMenu);
			action->setText(tr("Send message"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMAL_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,streamJid.full());
			action->setData(ADR_GROUP,groups);
			action->setData(ADR_CONTACT_JID,contacts);
			action->setShortcutId(SCT_ROSTERVIEW_SHOWNORMALDIALOG);
			AMenu->addAction(action,AG_RVCM_NORMALMESSAGEHANDLER,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void NormalMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
	IMessageWindow *messageWindow = findWindow(APresence->streamJid(),APresenceItem.itemJid);
	if (messageWindow)
		updateWindow(messageWindow);
}

void NormalMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AContext.isEmpty())
	{
		foreach (IMessageWindow *window, FWindows)
		{
			if (FLastMessages.value(window).type() == AMessageType)
			{
				IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
				if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
				{
					setMessageStyle(window);
					showStyledMessage(window,FLastMessages.value(window));
				}
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_normalmessagehandler, NormalMessageHandler)
