#include "joinmultichatdialog.h"

#include <QClipboard>
#include <QMessageBox>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/pluginhelper.h>
#include <utils/options.h>
#include <utils/logger.h>

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

JoinMultiChatDialog::JoinMultiChatDialog(IMultiUserChatManager *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Join conference"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_JOIN,0,0,"windowIcon");

	FMultiChatManager = AChatPlugin;

	FXmppStreamManager = PluginHelper::pluginInstance<IXmppStreamManager>();
	if (FXmppStreamManager)
	{
		foreach(IXmppStream *xmppStream, FXmppStreamManager->xmppStreams())
			if (FXmppStreamManager->isXmppStreamActive(xmppStream))
				onXmppStreamActiveChanged(xmppStream,true);

		ui.cmbStreamJid->model()->sort(0,Qt::AscendingOrder);
		ui.cmbStreamJid->setCurrentIndex(AStreamJid.isValid() ? ui.cmbStreamJid->findData(AStreamJid.pFull()) : 0);
		connect(ui.cmbStreamJid,SIGNAL(currentIndexChanged(int)),SLOT(onStreamIndexChanged(int)));

		connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamStateChanged(IXmppStream *)));
		connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamStateChanged(IXmppStream *)));
		connect(FXmppStreamManager->instance(),SIGNAL(streamJidChanged(IXmppStream *, const Jid &)),SLOT(onXmppStreamJidChanged(IXmppStream *, const Jid &)));
		connect(FXmppStreamManager->instance(),SIGNAL(streamActiveChanged(IXmppStream *, bool)),SLOT(onXmppStreamActiveChanged(IXmppStream *, bool)));
	}
	onStreamIndexChanged(ui.cmbStreamJid->currentIndex());

	if (ARoomJid.isValid())
	{
		ui.cmbHistory->setCurrentIndex(-1);
		ui.lneRoom->setText(ARoomJid.uNode());
		ui.lneService->setText(ARoomJid.domain());
		ui.lneNick->setText(!ANick.isEmpty() ? ANick : FStreamJid.uNode());
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

	connect(FMultiChatManager->instance(),SIGNAL(roomNickReceived(const Jid &, const Jid &, const QString &)),
		SLOT(onRoomNickReceived(const Jid &, const Jid &, const QString &)));
	connect(ui.cmbHistory,SIGNAL(currentIndexChanged(int)),SLOT(onHistoryIndexChanged(int)));
	connect(ui.tlbDeleteHistory,SIGNAL(clicked()),SLOT(onDeleteHistoryClicked()));
	connect(ui.tlbResolveNick,SIGNAL(clicked()),SLOT(onResolveNickClicked()));
	connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
}

JoinMultiChatDialog::~JoinMultiChatDialog()
{

}

void JoinMultiChatDialog::updateResolveNickState()
{
	IXmppStream *stream = FXmppStreamManager!=NULL ? FXmppStreamManager->findXmppStream(FStreamJid) : NULL;
	ui.tlbResolveNick->setEnabled(stream!=NULL && stream->isOpen());
}

void JoinMultiChatDialog::loadRecentConferences()
{
	QByteArray data = Options::fileValue("muc.joindialog.recent-rooms",FStreamJid.pBare()).toByteArray();
	QDataStream stream(data);
	stream >> FRecentRooms;

	QMultiMap<int, Jid> enters;
	foreach (const Jid &roomJid, FRecentRooms.keys())
		enters.insertMulti(FRecentRooms.value(roomJid).enters,roomJid);

	ui.cmbHistory->blockSignals(true);
	ui.cmbHistory->clear();
	foreach(const Jid &roomJid, enters)
	{
		RoomParams params = FRecentRooms.value(roomJid);
		ui.cmbHistory->addItem(tr("%1 as %2","room as nick").arg(roomJid.uBare()).arg(params.nick),roomJid.bare());
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
	Jid roomJid = Jid::fromUserInput(ui.lneRoom->text().trimmed() + "@" + ui.lneService->text().trimmed());
	QString nick = ui.lneNick->text();
	QString password = ui.lnePassword->text();

	if (FStreamJid.isValid() && roomJid.isValid() && !roomJid.node().isEmpty() && roomJid.resource().isEmpty())
	{
		IMultiUserChatWindow *chatWindow = FMultiChatManager->getMultiChatWindow(FStreamJid,roomJid,nick,password);
		if (chatWindow)
		{
			chatWindow->showTabPage();
			chatWindow->multiUserChat()->sendStreamPresence();
		}

		RoomParams &params = FRecentRooms[roomJid];
		params.enters++;
		params.nick = nick.isEmpty() ? FStreamJid.uNode() : nick;
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
	FStreamJid = ui.cmbStreamJid->itemData(AIndex).toString();
	updateResolveNickState();
	loadRecentConferences();
	onHistoryIndexChanged(ui.cmbHistory->currentIndex());

	QString buffer = QApplication::clipboard()->text().trimmed();
	QRegExp regexp = QRegExp(QLatin1String("^xmpp://(([^@]+)@([^/]+))"), Qt::CaseInsensitive, QRegExp::RegExp);

	if (regexp.exactMatch(buffer))
	{
		QStringList matches = regexp.capturedTexts();
		int cmbHistoryIndex = ui.cmbHistory->findData(Jid(matches.at(1)).pBare());
		if (cmbHistoryIndex == -1)
		{
			ui.lneRoom->setText(matches.at(2));
			ui.lneService->setText(matches.at(3));
		}
		else
		{
			ui.cmbHistory->setCurrentIndex(cmbHistoryIndex);
		}
	}
}

void JoinMultiChatDialog::onHistoryIndexChanged(int AIndex)
{
	Jid roomJid = ui.cmbHistory->itemData(AIndex).toString();
	if (FRecentRooms.contains(roomJid))
	{
		RoomParams params = FRecentRooms.value(roomJid);
		ui.lneRoom->setText(roomJid.uNode());
		ui.lneService->setText(roomJid.domain());
		ui.lneNick->setText(params.nick);
		ui.lnePassword->setText(params.password);
	}
}

void JoinMultiChatDialog::onResolveNickClicked()
{
	Jid serviceJid = ui.lneService->text().trimmed();
	IXmppStream *stream = FXmppStreamManager!=NULL ? FXmppStreamManager->findXmppStream(FStreamJid) : NULL;
	if (stream!=NULL && stream->isOpen() && serviceJid.isValid())
	{
		if (FMultiChatManager->requestRoomNick(FStreamJid,serviceJid))
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
		saveRecentConferences();
	}
}

void JoinMultiChatDialog::onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick)
{
	Jid serviceJid = ui.lneService->text().trimmed();
	if (AStreamJid==FStreamJid && ARoomJid==serviceJid)
	{
		if (ui.lneNick->text().isEmpty())
			ui.lneNick->setText(ANick.isEmpty() ? FStreamJid.uNode() : ANick);
		updateResolveNickState();
	}
}

void JoinMultiChatDialog::onXmppStreamStateChanged(IXmppStream *AXmppStream)
{
	if (AXmppStream->streamJid() == FStreamJid)
		updateResolveNickState();
}

void JoinMultiChatDialog::onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(ABefore.pFull()));
	ui.cmbStreamJid->addItem(AXmppStream->streamJid().uFull(),AXmppStream->streamJid().pFull());
}

void JoinMultiChatDialog::onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive)
{
	if (AActive)
		ui.cmbStreamJid->addItem(AXmppStream->streamJid().uFull(),AXmppStream->streamJid().pFull());
	else
		ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(AXmppStream->streamJid().pFull()));
}
