#ifndef SATEWIDGET_H
#define SATEWIDGET_H

#include <QLabel>
#include <QToolButton>
#include <interfaces/ichatstates.h>
#include <interfaces/imessagewidgets.h>

class StateWidget :
	public QToolButton
{
	Q_OBJECT;
public:
	StateWidget(IChatStates *AChatStates, IMessageChatWindow *AWindow, QWidget *AParent);
	~StateWidget();
protected slots:
	void onStatusActionTriggered(bool);
	void onPermitStatusChanged(const Jid &AContactJid, int AStatus);
	void onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState);
protected slots:
	void onWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	IMessageChatWindow *FWindow;
	IChatStates *FChatStates;
private:
	Menu *FMenu;
};

#endif // SATEWIDGET_H
