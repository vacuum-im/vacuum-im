#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <definitions/shortcuts.h>
#include <definitions/messagedataroles.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <utils/options.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include "ui_messagewindow.h"

class MessageWindow :
			public QMainWindow,
			public IMessageWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWindow ITabWindowPage);
public:
	MessageWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode);
	virtual ~MessageWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabWindowPage
	virtual QString tabPageId() const;
	virtual bool isActive() const;
	virtual void showWindow();
	virtual void closeWindow();
	//IMessageWindow
	virtual const Jid &streamJid() const { return FStreamJid; }
	virtual const Jid &contactJid() const { return FContactJid; }
	virtual void setContactJid(const Jid &AContactJid);
	virtual void addTabWidget(QWidget *AWidget);
	virtual void setCurrentTabWidget(QWidget *AWidget);
	virtual void removeTabWidget(QWidget *AWidget);
	virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
	virtual IViewWidget *viewWidget() const { return FViewWidget; }
	virtual IEditWidget *editWidget() const { return FEditWidget; }
	virtual IReceiversWidget *receiversWidget() const { return FReceiversWidget; }
	virtual IToolBarWidget *viewToolBarWidget() const { return FViewToolBarWidget; }
	virtual IToolBarWidget *editToolBarWidget() const { return FEditToolBarWidget; }
	virtual Mode mode() const { return FMode; }
	virtual void setMode(Mode AMode);
	virtual QString subject() const { return ui.lneSubject->text(); }
	virtual void setSubject(const QString &ASubject);
	virtual QString threadId() const { return FCurrentThreadId; }
	virtual void setThreadId(const QString &AThreadId);
	virtual int nextCount() const { return FNextCount; }
	virtual void setNextCount(int ACount);
	virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle);
signals:
	//ITabWindowPage
	void windowShow();
	void windowClose();
	void windowChanged();
	void windowActivated();
	void windowDeactivated();
	void windowDestroyed();
	//IMessageWindow
	void showNextMessage();
	void replyMessage();
	void forwardMessage();
	void showChatWindow();
	void messageReady();
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
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
	void onStreamJidChanged(const Jid &ABefore);
	void onMessageReady();
	void onSendButtonClicked();
	void onNextButtonClicked();
	void onReplyButtonClicked();
	void onForwardButtonClicked();
	void onChatButtonClicked();
	void onReceiversChanged(const Jid &AReceiver);
private:
	Ui::MessageWindowClass ui;
private:
	IInfoWidget *FInfoWidget;
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IReceiversWidget *FReceiversWidget;
	IToolBarWidget *FViewToolBarWidget;
	IToolBarWidget *FEditToolBarWidget;
private:
	IMessageWidgets *FMessageWidgets;
private:
	Mode FMode;
	int FNextCount;
	Jid FStreamJid;
	Jid FContactJid;
	bool FShownDetached;
	QString FCurrentThreadId;
};

#endif // MESSAGEWINDOW_H
