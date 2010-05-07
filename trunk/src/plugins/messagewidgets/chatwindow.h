#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <definations/optionvalues.h>
#include <definations/messagedataroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istatuschanger.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>
#include "ui_chatwindow.h"

class ChatWindow :
			public QMainWindow,
			public IChatWindow
{
	Q_OBJECT;
	Q_INTERFACES(IChatWindow ITabWindowPage);
public:
	ChatWindow(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	virtual ~ChatWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabWindowPage
	virtual QString tabPageId() const;
	virtual void showWindow();
	virtual void closeWindow();
	//IChatWindow
	virtual const Jid &streamJid() const { return FStreamJid; }
	virtual const Jid &contactJid() const { return FContactJid; }
	virtual void setContactJid(const Jid &AContactJid);
	virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
	virtual IViewWidget *viewWidget() const { return FViewWidget; }
	virtual IEditWidget *editWidget() const { return FEditWidget; }
	virtual IMenuBarWidget *menuBarWidget() const { return FMenuBarWidget; }
	virtual IToolBarWidget *toolBarWidget() const { return FToolBarWidget; }
	virtual IStatusBarWidget *statusBarWidget() const { return FStatusBarWidget; }
	virtual bool isActive() const;
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle);
signals:
	//ITabWindowPage
	void windowShow();
	void windowClose();
	void windowChanged();
	void windowDestroyed();
	//IChatWindow
	void messageReady();
	void streamJidChanged(const Jid &ABefour);
	void contactJidChanged(const Jid &ABefour);
	void windowActivated();
	void windowClosed();
protected:
	void initialize();
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onMessageReady();
	void onStreamJidChanged(const Jid &ABefour);
	void onOptionsChanged(const OptionsNode &ANode);
private:
	Ui::ChatWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
	IStatusChanger *FStatusChanger;
private:
	IInfoWidget *FInfoWidget;
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IMenuBarWidget *FMenuBarWidget;
	IToolBarWidget *FToolBarWidget;
	IStatusBarWidget *FStatusBarWidget;
private:
	Jid FStreamJid;
	Jid FContactJid;
	bool FShownDetached;
};

#endif // CHATWINDOW_H
