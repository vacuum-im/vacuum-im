#include "joinmultichatdialog.h"

#include <QMessageBox>

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

	ui.lneHost->setText(ARoomJid.domain().isEmpty() ? QString("conference.%1").arg(FStreamJid.domain()) : ARoomJid.domain());
	ui.lneRoom->setText(ARoomJid.node());
	ui.lnePassword->setText(APassword);
	ui.lneNick->setText(ANick.isEmpty() ? FStreamJid.node() : ANick);

	if (ANick.isEmpty())
		onResolveNickClicked();

	connect(FChatPlugin->instance(),SIGNAL(roomNickReceived(const Jid &, const Jid &, const QString &)),
	        SLOT(onRoomNickReceived(const Jid &, const Jid &, const QString &)));
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

void JoinMultiChatDialog::onDialogAccepted()
{
	QString host = ui.lneHost->text();
	QString room = ui.lneRoom->text();
	QString nick = ui.lneNick->text();
	QString password = ui.lnePassword->text();

	Jid roomJid(room,host,"");
	if (FStreamJid.isValid() && roomJid.isValid() && !host.isEmpty() && !room.isEmpty() && !nick.isEmpty())
	{
		IMultiUserChatWindow *chatWindow = FChatPlugin->getMultiChatWindow(FStreamJid,roomJid,nick,password);
		if (chatWindow)
		{
			chatWindow->showWindow();
			chatWindow->multiUserChat()->setAutoPresence(true);
		}
		accept();
	}
	else
	{
		QMessageBox::warning(this,windowTitle(),tr("Room parameters is not acceptable.\nCheck values and try again"));
	}
}

void JoinMultiChatDialog::onStreamIndexChanged(int AIndex)
{
	FStreamJid = ui.cmbStreamJid->itemText(AIndex);
	updateResolveNickState();
}

void JoinMultiChatDialog::onResolveNickClicked()
{
	Jid roomJid(ui.lneRoom->text(),ui.lneHost->text(),"");
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(FStreamJid) : NULL;
	if (stream!=NULL && stream->isOpen() && roomJid.isValid())
	{
		if (FChatPlugin->requestRoomNick(FStreamJid,roomJid))
		{
			ui.lneNick->clear();
			ui.tlbResolveNick->setEnabled(false);
		}
	}
}

void JoinMultiChatDialog::onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick)
{
	Jid roomJid(ui.lneRoom->text(),ui.lneHost->text(),"");
	if (AStreamJid == FStreamJid && ARoomJid == roomJid)
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

void JoinMultiChatDialog::onStreamStateChanged(IXmppStream * AXmppStream)
{
	if (AXmppStream->streamJid() == FStreamJid)
		updateResolveNickState();
}

void JoinMultiChatDialog::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(ABefour.pFull()));
	onStreamAdded(AXmppStream);
}

void JoinMultiChatDialog::onStreamRemoved(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(AXmppStream->streamJid().pFull()));
}
