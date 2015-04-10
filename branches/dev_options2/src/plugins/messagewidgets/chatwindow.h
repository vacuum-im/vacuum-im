#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/ixmppstreammanager.h>
#include "ui_chatwindow.h"

class ChatWindow :
	public QMainWindow,
	public IMessageChatWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWindow IMessageChatWindow IMessageTabPage);
public:
	ChatWindow(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	virtual ~ChatWindow();
	virtual QMainWindow *instance() { return this; }
	// IMessageWindow
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual IMessageAddress *address() const;
	virtual IMessageInfoWidget *infoWidget() const;
	virtual IMessageViewWidget *viewWidget() const;
	virtual IMessageEditWidget *editWidget() const;
	virtual IMessageMenuBarWidget *menuBarWidget() const;
	virtual IMessageToolBarWidget *toolBarWidget() const;
	virtual IMessageStatusBarWidget *statusBarWidget() const;
	virtual IMessageReceiversWidget *receiversWidget() const;
	// IMessageTabPage
	virtual QString tabPageId() const;
	virtual bool isVisibleTabPage() const;
	virtual bool isActiveTabPage() const;
	virtual void assignTabPage();
	virtual void showTabPage();
	virtual void showMinimizedTabPage();
	virtual void closeTabPage();
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	virtual IMessageTabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(IMessageTabPageNotifier *ANotifier);
	// IMessageChatWindow
	virtual SplitterWidget *messageWidgetsBox() const;
	virtual void updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip);
signals:
	// IMessageWindow
	void widgetLayoutChanged();
	// ITabWindowPage
	void tabPageAssign();
	void tabPageShow();
	void tabPageShowMinimized();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
protected:
	void saveWindowGeometryAndState();
	void loadWindowGeometryAndState();
protected:
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	Ui::ChatWindowClass ui;
private:
	IMessageAddress *FAddress;
	IMessageInfoWidget *FInfoWidget;
	IMessageViewWidget *FViewWidget;
	IMessageEditWidget *FEditWidget;
	IMessageMenuBarWidget *FMenuBarWidget;
	IMessageToolBarWidget *FToolBarWidget;
	IMessageStatusBarWidget *FStatusBarWidget;
	IMessageTabPageNotifier *FTabPageNotifier;
	IMessageWidgets *FMessageWidgets;
private:
	bool FShownDetached;
	QString FTabPageToolTip;
};

#endif // CHATWINDOW_H
