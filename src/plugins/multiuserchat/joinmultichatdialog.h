#ifndef JOINMULTICHATDIALOG_H
#define JOINMULTICHATDIALOG_H

#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ixmppstreams.h>
#include "ui_joinmultichatdialog.h"

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
protected slots:
	void onDialogAccepted();
	void onStreamIndexChanged(int AIndex);
	void onResolveNickClicked();
	void onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick);
	void onStreamAdded(IXmppStream *AXmppStream);
	void onStreamStateChanged(IXmppStream *AXmppStream);
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
	void onStreamRemoved(IXmppStream *AXmppStream);
private:
	Ui::JoinMultiChatDialogClass ui;
private:
	IXmppStreams *FXmppStreams;
	IMultiUserChatPlugin *FChatPlugin;
private:
	Jid FStreamJid;
};

#endif // JOINMULTICHATDIALOG_H
