#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <definitions/shortcuts.h>
#include <definitions/messagedataroles.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/xmpperror.h>
#include <utils/widgetmanager.h>
#include "ui_normalwindow.h"

class NormalWindow :
	public QMainWindow,
	public IMessageNormalWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWindow IMessageNormalWindow IMessageTabPage);
public:
	NormalWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode);
	virtual ~NormalWindow();
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
	// ITabWindowPage
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
	// IMessageNormalWindow
	virtual void addTabWidget(QWidget *AWidget);
	virtual void setCurrentTabWidget(QWidget *AWidget);
	virtual void removeTabWidget(QWidget *AWidget);
	virtual Mode mode() const { return FMode; }
	virtual void setMode(Mode AMode);
	virtual QString subject() const;
	virtual void setSubject(const QString &ASubject);
	virtual QString threadId() const;
	virtual void setThreadId(const QString &AThreadId);
	virtual int nextCount() const;
	virtual void setNextCount(int ACount);
	virtual IMessageToolBarWidget *viewToolBarWidget() const;
	virtual IMessageToolBarWidget *editToolBarWidget() const;
	virtual void updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip);
signals:
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
	// IMessageNormalWindow
	void showNextMessage();
	void replyMessage();
	void forwardMessage();
	void showChatWindow();
	void messageReady();
protected:
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void closeEvent(QCloseEvent *AEvent);
protected slots:
	void onMessageReady();
	void onSendButtonClicked();
	void onNextButtonClicked();
	void onReplyButtonClicked();
	void onForwardButtonClicked();
	void onChatButtonClicked();
	void onReceiversChanged(const Jid &AReceiver);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	Ui::NormalWindowClass ui;
private:
	IMessageAddress *FAddress;
	IMessageInfoWidget *FInfoWidget;
	IMessageViewWidget *FViewWidget;
	IMessageEditWidget *FEditWidget;
	IMessageReceiversWidget *FReceiversWidget;
	IMessageMenuBarWidget *FMenuBarWidget;
	IMessageToolBarWidget *FViewToolBarWidget;
	IMessageToolBarWidget *FEditToolBarWidget;
	IMessageStatusBarWidget *FStatusBarWidget;
	IMessageTabPageNotifier *FTabPageNotifier;
	IMessageWidgets *FMessageWidgets;
private:
	Mode FMode;
	int FNextCount;
	bool FShownDetached;
	QString FTabPageToolTip;
	QString FCurrentThreadId;
};

#endif // MESSAGEWINDOW_H
