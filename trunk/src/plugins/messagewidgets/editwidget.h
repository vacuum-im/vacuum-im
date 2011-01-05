#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include "ui_editwidget.h"

class EditWidget :
			public QWidget,
			public IEditWidget
{
	Q_OBJECT;
	Q_INTERFACES(IEditWidget);
public:
	EditWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	~EditWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const;
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QTextEdit *textEdit() const;
	virtual QTextDocument *document() const;
	virtual void sendMessage();
	virtual void clearEditor();
	virtual bool autoResize() const;
	virtual void setAutoResize(bool AResize);
	virtual int minimumLines() const;
	virtual void setMinimumLines(int ALines);
	virtual QString sendShortcut() const;
	virtual void setSendShortcut(const QString &AShortcutId);
	virtual bool sendButtonVisible() const;
	virtual void setSendButtonVisible(bool AVisible);
	virtual bool textFormatEnabled() const;
	virtual void setTextFormatEnabled(bool AEnabled);
signals:
	void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
	void messageAboutToBeSend();
	void messageReady();
	void editorCleared();
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
	void autoResizeChanged(bool AResize);
	void minimumLinesChanged(int ALines);
	void sendShortcutChanged(const QString &AShortcutId);
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void appendMessageToBuffer();
	void showBufferedMessage();
	void showNextBufferedMessage();
	void showPrevBufferedMessage();
protected slots:
	void onSendButtonCliked(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onOptionsChanged(const OptionsNode &ANode);
	void onContentsChanged(int APosition, int ARemoved, int AAdded);
private:
	Ui::EditWidgetClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	bool FFormatEnabled;
	int FBufferPos;
	Jid FStreamJid;
	Jid FContactJid;
	QString FShortcutId;
	QList<QString> FBuffer;
};

#endif // EDITWIDGET_H
