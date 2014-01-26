#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/toolbarchanger.h>
#include "ui_editwidget.h"

class EditWidget :
			public QWidget,
			public IEditWidget
{
	Q_OBJECT;
	Q_INTERFACES(IEditWidget);
public:
	EditWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
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
	virtual bool sendToolBarVisible() const;
	virtual void setSendToolBarVisible(bool AVisible);
	virtual ToolBarChanger *sendToolBarChanger() const;
	virtual bool isRichTextEnabled() const;
	virtual void setRichTextEnabled(bool AEnabled);
	virtual void insertTextFragment(const QTextDocumentFragment &AFragment);
	virtual QTextDocumentFragment prepareTextFragment(const QTextDocumentFragment &AFragment) const;
signals:
	// IEditWidget
	void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
	void messageAboutToBeSend();
	void messageReady();
	void editorCleared();
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
	void autoResizeChanged(bool AResize);
	void minimumLinesChanged(int ALines);
	void sendShortcutChanged(const QString &AShortcutId);
	void richTextEnableChanged(bool AEnabled);
	// EditWidget
	void createDataRequest(QMimeData *ADestination) const;
	void canInsertDataRequest(const QMimeData *AData, bool &ACanInsert) const;
	void insertDataRequest(const QMimeData *AData, QTextDocument *ADocument) const;
	void contentsChanged(int APosition, int ARemoved, int AAdded) const;
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void appendMessageToBuffer();
	void showBufferedMessage();
	void showNextBufferedMessage();
	void showPrevBufferedMessage();
protected slots:
	void onUpdateSendToolBarMaxWidth();
	void onSendActionTriggered(bool);
	void onShortcutUpdated(const QString &AId);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onOptionsChanged(const OptionsNode &ANode);
	void onEditorCreateDataRequest(QMimeData *AData);
	void onEditorCanInsertDataRequest(const QMimeData *AData, bool &ACanInsert);
	void onEditorInsertDataRequest(const QMimeData *AData, QTextDocument *ADocument);
	void onEditorContentsChanged(int APosition, int ARemoved, int AAdded);
private:
	Ui::EditWidgetClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	int FBufferPos;
	Jid FStreamJid;
	Jid FContactJid;
	QList<QString> FBuffer;
	QString FSendShortcutId;
	QKeySequence FSendShortcut;
	ToolBarChanger *FSendToolBar;
};

#endif // EDITWIDGET_H
