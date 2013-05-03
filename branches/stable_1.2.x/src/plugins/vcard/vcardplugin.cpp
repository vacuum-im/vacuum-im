#include "vcardplugin.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>

#define DIR_VCARDS                "vcards"
#define VCARD_TIMEOUT             60000

#define UPDATE_VCARD_DAYS         7
#define UPDATE_REQUEST_TIMEOUT    5000

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

VCardPlugin::VCardPlugin()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FRosterPlugin = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FStanzaProcessor = NULL;
	FMultiUserChatPlugin = NULL;
	FDiscovery = NULL;
	FXmppUriQueries = NULL;
	FMessageWidgets = NULL;

	FUpdateTimer.setSingleShot(false);
	FUpdateTimer.start(UPDATE_REQUEST_TIMEOUT);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));
}

VCardPlugin::~VCardPlugin()
{

}

void VCardPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Visit Card Manager");
	APluginInfo->description = tr("Allows to obtain personal contact information");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool VCardPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onXmppStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
		if (FMultiUserChatPlugin)
		{
			connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(), SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
		}
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return true;
}

bool VCardPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SHOWVCARD, tr("Show vCard"), tr("Ctrl+I","Show vCard"));
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWVCARD, tr("Show vCard"), tr("Ctrl+I","Show vCard"), Shortcuts::WidgetShortcut);

	if (FRostersView)
	{
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWVCARD,FRostersView->instance());
	}
	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	return true;
}

void VCardPlugin::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FVCardRequestId.contains(AStanza.id()))
	{
		Jid fromJid = FVCardRequestId.take(AStanza.id());
		QDomElement elem = AStanza.firstElement(VCARD_TAGNAME,NS_VCARD_TEMP);
		if (AStanza.type() == "result")
		{
			saveVCardFile(fromJid,elem);
			emit vcardReceived(fromJid);
		}
		else if (AStanza.type() == "error")
		{
			saveVCardFile(fromJid,QDomElement());
			XmppStanzaError err(AStanza);
			emit vcardError(fromJid,err.errorMessage());
		}
	}
	else if (FVCardPublishId.contains(AStanza.id()))
	{
		Jid streamJid = FVCardPublishId.take(AStanza.id());
		Stanza stanza = FVCardPublishStanza.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			saveVCardFile(streamJid,stanza.element().firstChildElement(VCARD_TAGNAME));
			emit vcardPublished(streamJid);
		}
		else if (AStanza.type() == "error")
		{
			emit vcardError(streamJid,XmppStanzaError(AStanza).errorMessage());
		}
	}
}

bool VCardPlugin::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	Q_UNUSED(AParams);
	if (AAction == "vcard")
	{
		showVCardDialog(AStreamJid, AContactJid);
		return true;
	}
	return false;
}

QString VCardPlugin::vcardFileName(const Jid &AContactJid) const
{
	static bool entered = false;
	static QDir dir(FPluginManager->homePath());
	
	if (!entered)
	{
		entered = true;
		if (!dir.exists(DIR_VCARDS))
			dir.mkdir(DIR_VCARDS);
		dir.cd(DIR_VCARDS);
	}
	
	return dir.absoluteFilePath(Jid::encode(AContactJid.pFull())+".xml");
}

bool VCardPlugin::hasVCard(const Jid &AContactJid) const
{
	return QFile::exists(vcardFileName(AContactJid));
}

IVCard *VCardPlugin::vcard(const Jid &AContactJid)
{
	VCardItem &vcardItem = FVCards[AContactJid];
	if (vcardItem.vcard == NULL)
		vcardItem.vcard = new VCard(this,AContactJid);
	vcardItem.locks++;
	return vcardItem.vcard;
}

bool VCardPlugin::requestVCard(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FStanzaProcessor)
	{
		if (FVCardRequestId.key(AContactJid).isEmpty())
		{
			Stanza request("iq");
			request.setTo(AContactJid.full()).setType("get").setId(FStanzaProcessor->newId());
			request.addElement(VCARD_TAGNAME,NS_VCARD_TEMP);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,VCARD_TIMEOUT))
			{
				FVCardRequestId.insert(request.id(),AContactJid);
				return true;
			}
			return false;
		}
		return true;
	}
	return false;
}

bool VCardPlugin::publishVCard(IVCard *AVCard, const Jid &AStreamJid)
{
	if (FStanzaProcessor && AVCard->isValid() && FVCardPublishId.key(AStreamJid.pBare()).isEmpty())
	{
		Stanza publish("iq");
		publish.setTo(AStreamJid.bare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = publish.element().appendChild(AVCard->vcardElem().cloneNode(true)).toElement();
		removeEmptyChildElements(elem);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,publish,VCARD_TIMEOUT))
		{
			FVCardPublishId.insert(publish.id(),AStreamJid.pBare());
			FVCardPublishStanza.insert(publish.id(),publish);
			return true;
		}
	}
	return false;
}

void VCardPlugin::showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FVCardDialogs.contains(AContactJid))
	{
		VCardDialog *dialog = FVCardDialogs.value(AContactJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
	else if (AStreamJid.isValid() && AContactJid.isValid())
	{
		VCardDialog *dialog = new VCardDialog(this,AStreamJid,AContactJid);
		connect(dialog,SIGNAL(destroyed(QObject *)),SLOT(onVCardDialogDestroyed(QObject *)));
		FVCardDialogs.insert(AContactJid,dialog);
		dialog->show();
	}
}

void VCardPlugin::unlockVCard(const Jid &AContactJid)
{
	VCardItem &vcardItem = FVCards[AContactJid];
	vcardItem.locks--;
	if (vcardItem.locks <= 0)
	{
		VCard *vcardCopy = vcardItem.vcard;   //После remove vcardItem будет недействителен
		FVCards.remove(AContactJid);
		delete vcardCopy;
	}
}

void VCardPlugin::saveVCardFile(const Jid &AContactJid,const QDomElement &AElem) const
{
	if (AContactJid.isValid())
	{
		QDomDocument doc;
		QDomElement rootElem = doc.appendChild(doc.createElement(VCARD_TAGNAME)).toElement();
		rootElem.setAttribute("jid",AContactJid.full());
		rootElem.setAttribute("dateTime",QDateTime::currentDateTime().toString(Qt::ISODate));

		QFile file(vcardFileName(AContactJid));
		if (!AElem.isNull() && file.open(QIODevice::WriteOnly|QIODevice::Truncate))
		{
			rootElem.appendChild(AElem.cloneNode(true));
			file.write(doc.toByteArray());
			file.close();
		}
		else if (AElem.isNull() && !file.exists() && file.open(QIODevice::WriteOnly|QIODevice::Truncate))
		{
			file.write(doc.toByteArray());
			file.close();
		}
		else if (AElem.isNull() && file.exists() && file.open(QIODevice::ReadWrite))
		{
			char data;
			if (file.getChar(&data))
			{
				file.seek(0);
				file.putChar(data);
			}
			file.close();
		}
	}
}

void VCardPlugin::removeEmptyChildElements(QDomElement &AElem) const
{
	static const QStringList tagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400" << "CELL" << "MODEM";

	QDomElement curChild = AElem.firstChildElement();
	while (!curChild.isNull())
	{
		removeEmptyChildElements(curChild);
		QDomElement nextChild = curChild.nextSiblingElement();
		if (curChild.text().isEmpty() && !tagList.contains(curChild.tagName()))
			curChild.parentNode().removeChild(curChild);
		curChild = nextChild;
	}
}

void VCardPlugin::registerDiscoFeatures()
{
	IDiscoFeature dfeature;

	dfeature.active = false;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_VCARD);
	dfeature.var = NS_VCARD_TEMP;
	dfeature.name = tr("Visit Card");
	dfeature.description = tr("Supports the requesting of the personal contact information");
	FDiscovery->insertDiscoFeature(dfeature);
}

void VCardPlugin::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance() && !FRostersView->hasMultiSelection())
	{
		if (AId == SCT_ROSTERVIEW_SHOWVCARD)
		{
			QModelIndex index = FRostersView->instance()->currentIndex();
			int indexType = index.data(RDR_TYPE).toInt();
			if (indexType==RIT_STREAM_ROOT || indexType==RIT_CONTACT || indexType==RIT_AGENT)
			{
				showVCardDialog(index.data(RDR_STREAM_JID).toString(),index.data(RDR_PREP_BARE_JID).toString());
			}
		}
	}
}

void VCardPlugin::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if (ALabelId==RLID_DISPLAY && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		if (index->type() == RIT_STREAM_ROOT || index->type() == RIT_CONTACT || index->type() == RIT_AGENT)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Show vCard"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
			action->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,Jid(index->data(RDR_FULL_JID).toString()).bare());
			action->setShortcutId(SCT_ROSTERVIEW_SHOWVCARD);
			AMenu->addAction(action,AG_RVCM_VCARD,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
		}
	}
}

void VCardPlugin::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Q_UNUSED(AWindow);
	Action *action = new Action(AMenu);
	action->setText(tr("Show vCard"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
	action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAM_JID));
	if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
		action->setData(ADR_CONTACT_JID,Jid(AUser->data(MUDR_REAL_JID).toString()).bare());
	else
		action->setData(ADR_CONTACT_JID,AUser->data(MUDR_CONTACT_JID));
	AMenu->addAction(action,AG_MUCM_VCARD,true);
	connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
}

void VCardPlugin::onShowVCardDialogByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		showVCardDialog(streamJid,contactJid);
	}
}

void VCardPlugin::onShowVCardDialogByChatWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IToolBarWidget *toolBarWidget = qobject_cast<IToolBarWidget *>(action->parent());
		if (toolBarWidget && toolBarWidget->viewWidget())
		{
			bool isMucUser = false;
			Jid contactJid = toolBarWidget->viewWidget()->contactJid();
			QList<IMultiUserChatWindow *> windows = FMultiUserChatPlugin!=NULL ? FMultiUserChatPlugin->multiChatWindows() : QList<IMultiUserChatWindow *>();
			for (int i=0; !isMucUser && i<windows.count(); i++)
				isMucUser = windows.at(i)->findChatWindow(contactJid)!=NULL;
			showVCardDialog(toolBarWidget->viewWidget()->streamJid(), isMucUser ? contactJid : contactJid.bare());
		}
	}
}

void VCardPlugin::onVCardDialogDestroyed(QObject *ADialog)
{
	VCardDialog *dialog = static_cast<VCardDialog *>(ADialog);
	FVCardDialogs.remove(FVCardDialogs.key(dialog));
}

void VCardPlugin::onXmppStreamRemoved(IXmppStream *AXmppStream)
{
	foreach(VCardDialog *dialog, FVCardDialogs.values())
		if (dialog->streamJid() == AXmppStream->streamJid())
			delete dialog;
}

void VCardPlugin::onChatWindowCreated(IChatWindow *AWindow)
{
	if (AWindow->toolBarWidget() && AWindow->toolBarWidget()->viewWidget())
	{
		Action *action = new Action(AWindow->toolBarWidget()->instance());
		action->setText(tr("Show vCard"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
		action->setShortcutId(SCT_MESSAGEWINDOWS_SHOWVCARD);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByChatWindowAction(bool)));
		AWindow->toolBarWidget()->toolBarChanger()->insertAction(action,TBG_MWTBW_VCARD_VIEW);
	}
}

void VCardPlugin::onUpdateTimerTimeout()
{
	bool requestSent = false;
	QMultiMap<Jid,Jid>::iterator it=FUpdateQueue.begin();
	while(!requestSent && it!=FUpdateQueue.end())
	{
		QFileInfo info(vcardFileName(it.value()));
		if (!info.exists() || info.lastModified().daysTo(QDateTime::currentDateTime())>UPDATE_VCARD_DAYS)
			requestSent = requestVCard(it.key(),it.value());
		it = FUpdateQueue.erase(it);
	}
}

void VCardPlugin::onRosterOpened(IRoster *ARoster)
{
	IRosterItem emptyItem;
	foreach(const IRosterItem &item, ARoster->rosterItems())
		onRosterItemReceived(ARoster,item,emptyItem);
}

void VCardPlugin::onRosterClosed(IRoster *ARoster)
{
	FUpdateQueue.remove(ARoster->streamJid());
}

void VCardPlugin::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (ARoster->isOpen() && !ABefore.isValid)
	{
		if (!FUpdateQueue.contains(ARoster->streamJid(),AItem.itemJid))
			FUpdateQueue.insertMulti(ARoster->streamJid(),AItem.itemJid);
	}
}

Q_EXPORT_PLUGIN2(plg_vcard, VCardPlugin)
