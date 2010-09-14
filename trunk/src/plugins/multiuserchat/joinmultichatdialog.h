#ifndef JOINMULTICHATDIALOG_H
#define JOINMULTICHATDIALOG_H

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ixmppstreams.h>
#include <utils/options.h>
#include "ui_joinmultichatdialog.h"

struct RoomParams
{
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
	JoinMultiChatDialog(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid,
	                    const QString &ANick, const QString &APassword, QWidget *AParent = NULL);
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
	void onStreamAdded(IXmppStream *AXmppStream);
	void onStreamStateChanged(IXmppStream *AXmppStream);
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onStreamRemoved(IXmppStream *AXmppStream);
private:
	Ui::JoinMultiChatDialogClass ui;
private:
	IXmppStreams *FXmppStreams;
	IMultiUserChatPlugin *FChatPlugin;
private:
	Jid FStreamJid;
	QMap<Jid, RoomParams> FRecentRooms;
};

#endif // JOINMULTICHATDIALOG_H
