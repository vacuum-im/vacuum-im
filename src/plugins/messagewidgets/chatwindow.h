#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/messagedataroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istatuschanger.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/widgetmanager.h>
#include "ui_chatwindow.h"

class ChatWindow :
	public QMainWindow,
	public IChatWindow
{
	Q_OBJECT;
	Q_INTERFACES(IChatWindow ITabPage);
public:
	ChatWindow(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	virtual ~ChatWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabWindowPage
	virtual QString tabPageId() const;
	virtual bool isActiveTabPage() const;
	virtual void assignTabPage();
	virtual void showTabPage();
	virtual void showMinimizedTabPage();
	virtual void closeTabPage();
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
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
	virtual void updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip);
signals:
	//ITabWindowPage
	void tabPageAssign();
	void tabPageShow();
	void tabPageShowMinimized();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	//IChatWindow
	void messageReady();
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
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
	void onStreamJidChanged(const Jid &ABefore);
	void onOptionsChanged(const OptionsNode &ANode);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
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
	QString FTabPageToolTip;
};

#endif // CHATWINDOW_H
