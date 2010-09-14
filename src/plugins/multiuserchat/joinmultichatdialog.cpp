#include "joinmultichatdialog.h"

#include <QMessageBox>

QDataStream &operator<<(QDataStream &AStream, const RoomParams &AParams)
{
	AStream << AParams.enters;
	AStream << AParams.nick;
	AStream << AParams.password;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, RoomParams &AParams)
{
	AStream >> AParams.enters;
	AStream >> AParams.nick;
	AStream >> AParams.password;
	return AStream;
}

JoinMultiChatDialog::JoinMultiChatDialog(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid,
    const QString &ANick, const QString &APassword, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Join conference"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_JOIN,0,0,"windowIcon");

	FXmppStreams = NULL;
	FChatPlugin = AChatPlugin;
	initialize();

	if (FXmppStreams)
	{
		foreach(IXmppStream *xmppStream, FXmppStreams->xmppStreams())
			if (FXmppStreams->isActive(xmppStream))
				onStreamAdded(xmppStream);
		ui.cmbStreamJid->model()->sort(0,Qt::AscendingOrder);
		ui.cmbStreamJid->setCurrentIndex(AStreamJid.isValid() ? ui.cmbStreamJid->findData(AStreamJid.pFull()) : 0);
		connect(ui.cmbStreamJid,SIGNAL(currentIndexChanged(int)),SLOT(onStreamIndexChanged(int)));
	}
	onStreamIndexChanged(ui.cmbStreamJid->currentIndex());

	if (ARoomJid.isValid())
	{
		ui.cmbHistory->setCurrentIndex(-1);
		ui.lneRoom->setText(ARoomJid.node());
		ui.lneService->setText(ARoomJid.domain());
		ui.lneNick->setText(!ANick.isEmpty() ? ANick : FStreamJid.node());
		ui.lnePassword->setText(APassword);
	}
	else if (FRecentRooms.isEmpty())
	{
		ui.lneRoom->setText("vacuum");
		ui.lneService->setText("conference.jabber.ru");
	}

	if (ARoomJid.isValid() && ANick.isEmpty())
		onResolveNickClicked();
	else if (!ARoomJid.isValid() && FRecentRooms.isEmpty())
		onResolveNickClicked();

	connect(FChatPlugin->instance(),SIGNAL(roomNickReceived(const Jid &, const Jid &, const QString &)),
		SLOT(onRoomNickReceived(const Jid &, const Jid &, const QString &)));
	connect(ui.cmbHistory,SIGNAL(currentIndexChanged(int)),SLOT(onHistoryIndexChanged(int)));
	connect(ui.tlbDeleteHistory,SIGNAL(clicked()),SLOT(onDeleteHistoryClicked()));
	connect(ui.tlbResolveNick,SIGNAL(clicked()),SLOT(onResolveNickClicked()));
	connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
}

JoinMultiChatDialog::~JoinMultiChatDialog()
{

}

void JoinMultiChatDialog::initialize()
{
	IPlugin *plugin = FChatPlugin->pluginManager()->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamStateChanged(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamStateChanged(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}
}

void JoinMultiChatDialog::updateResolveNickState()
{
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(FStreamJid) : NULL;
	ui.tlbResolveNick->setEnabled(stream!=NULL && stream->isOpen());
}

void JoinMultiChatDialog::loadRecentConferences()
{
	QByteArray data = Options::fileValue("muc.joindialog.recent-rooms",FStreamJid.pBare()).toByteArray();
	QDataStream stream(data);
	stream >> FRecentRooms;

	QMultiMap<int, Jid> enters;
	foreach (Jid roomJid, FRecentRooms.keys())
		enters.insertMulti(FRecentRooms.value(roomJid).enters,roomJid);

	ui.cmbHistory->blockSignals(true);
	ui.cmbHistory->clear();
	foreach(Jid roomJid, enters)
	{
		RoomParams params = FRecentRooms.value(roomJid);
		ui.cmbHistory->addItem(tr("%1 as %2","room as nick").arg(roomJid.bare()).arg(params.nick),roomJid.bare());
	}
	ui.cmbHistory->blockSignals(false);
	ui.tlbDeleteHistory->setEnabled(!FRecentRooms.isEmpty());
}

void JoinMultiChatDialog::saveRecentConferences()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FRecentRooms;
	Options::setFileValue(data,"muc.joindialog.recent-rooms",FStreamJid.pBare());
}

void JoinMultiChatDialog::onDialogAccepted()
{
	Jid roomJid = ui.lneRoom->text().trimmed() + "@" + ui.lneService->text().trimmed();
	QString nick = ui.lneNick->text();
	QString password = ui.lnePassword->text();

	if (FStreamJid.isValid() && roomJid.isValid() && !roomJid.node().isEmpty() && roomJid.resource().isEmpty() && !nick.isEmpty())
	{
		IMultiUserChatWindow *chatWindow = FChatPlugin->getMultiChatWindow(FStreamJid,roomJid,nick,password);
		if (chatWindow)
		{
			chatWindow->showWindow();
			chatWindow->multiUserChat()->setAutoPresence(true);
		}

		RoomParams &params = FRecentRooms[roomJid];
		params.enters++;
		params.nick = nick;
		params.password = password;
		saveRecentConferences();

		accept();
	}
	else
	{
		QMessageBox::warning(this,windowTitle(),tr("Conference parameters is not acceptable.\nCheck values and try again"));
	}
}

void JoinMultiChatDialog::onStreamIndexChanged(int AIndex)
{
	FStreamJid = ui.cmbStreamJid->itemText(AIndex);
	updateResolveNickState();
	loadRecentConferences();
	onHistoryIndexChanged(ui.cmbHistory->currentIndex());
}

void JoinMultiChatDialog::onHistoryIndexChanged(int AIndex)
{
	Jid roomJid = ui.cmbHistory->itemData(AIndex).toString();
	if (FRecentRooms.contains(roomJid))
	{
		RoomParams params = FRecentRooms.value(roomJid);
		ui.lneRoom->setText(roomJid.node());
		ui.lneService->setText(roomJid.domain());
		ui.lneNick->setText(params.nick);
		ui.lnePassword->setText(params.password);
	}
}

void JoinMultiChatDialog::onResolveNickClicked()
{
	Jid serviceJid = ui.lneService->text().trimmed();
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(FStreamJid) : NULL;
	if (stream!=NULL && stream->isOpen() && serviceJid.isValid())
	{
		if (FChatPlugin->requestRoomNick(FStreamJid,serviceJid))
		{
			ui.lneNick->clear();
			ui.tlbResolveNick->setEnabled(false);
		}
	}
}

void JoinMultiChatDialog::onDeleteHistoryClicked()
{
	Jid roomJid = ui.cmbHistory->itemData(ui.cmbHistory->currentIndex()).toString();
	if (FRecentRooms.contains(roomJid))
	{
		FRecentRooms.remove(roomJid);
		ui.cmbHistory->removeItem(ui.cmbHistory->currentIndex());
		ui.tlbDeleteHistory->setEnabled(!FRecentRooms.isEmpty());
	}
}

void JoinMultiChatDialog::onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick)
{
	Jid serviceJid = ui.lneService->text().trimmed();
	if (AStreamJid==FStreamJid && ARoomJid==serviceJid)
	{
		if (ui.lneNick->text().isEmpty())
			ui.lneNick->setText(ANick.isEmpty() ? FStreamJid.node() : ANick);
		updateResolveNickState();
	}
}

void JoinMultiChatDialog::onStreamAdded(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->addItem(AXmppStream->streamJid().full(), AXmppStream->streamJid().pFull());
}

void JoinMultiChatDialog::onStreamStateChanged(IXmppStream *AXmppStream)
{
	if (AXmppStream->streamJid() == FStreamJid)
		updateResolveNickState();
}

void JoinMultiChatDialog::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(ABefore.pFull()));
	onStreamAdded(AXmppStream);
}

void JoinMultiChatDialog::onStreamRemoved(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(AXmppStream->streamJid().pFull()));
}
