#ifndef JOINMULTICHATDIALOG_H
#define JOINMULTICHATDIALOG_H

#include <interfaces/imultiuserchat.h>
#include <interfaces/ixmppstreammanager.h>
#include "ui_joinmultichatdialog.h"

struct RoomParams {
	RoomParams() {
		enters = 0;
	};
	int enters;
	QString nick;
	QString password;
};

class JoinMultiChatDialog :
	public QDialog
{
	Q_OBJECT;
public:
	JoinMultiChatDialog(IMultiUserChatManager *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent = NULL);
	~JoinMultiChatDialog();
protected:
	void initialize();
	void updateResolveNickState();
	void loadRecentConferences();
	void saveRecentConferences();
protected slots:
	void onDialogAccepted();
	void onStreamIndexChanged(int AIndex);
	void onHistoryIndexChanged(int AIndex);
	void onResolveNickClicked();
	void onDeleteHistoryClicked();
	void onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick);
	void onXmppStreamStateChanged(IXmppStream *AXmppStream);
	void onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive);
private:
	Ui::JoinMultiChatDialogClass ui;
private:
	IXmppStreamManager *FXmppStreamManager;
	IMultiUserChatManager *FMultiChatManager;
private:
	Jid FStreamJid;
	QMap<Jid, RoomParams> FRecentRooms;
};

#endif // JOINMULTICHATDIALOG_H
