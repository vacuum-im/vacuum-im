#ifndef SATEWIDGET_H
#define SATEWIDGET_H

#include <QLabel>
#include <QToolButton>
#include <interfaces/ichatstates.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>

class StateWidget :
	public QToolButton
{
	Q_OBJECT;
public:
	StateWidget(IChatStates *AChatStates, IMessageWindow *AWindow, QWidget *AParent);
	~StateWidget();
protected slots:
	void onStatusActionTriggered(QAction*);
	void onPermitStatusChanged(const Jid &AContactJid, int AStatus);
	void onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState);
	void onUserRoomStateChanged(const Jid &AStreamJid, const Jid &AUserJid, int AState);
protected slots:
	void onWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	IChatStates *FChatStates;
	IMessageWindow *FWindow;
	IMultiUserChatWindow *FMultiWindow;
private:
	Menu *FMenu;
	QSet<Jid> FActive;
	QSet<Jid> FPaused;
	QSet<Jid> FComposing;

};

#endif // SATEWIDGET_H
