#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/toolbargroups.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/toolbarchanger.h>
#include "ui_editwidget.h"

class EditWidget :
	public QWidget,
	public IMessageEditWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageEditWidget);
public:
	EditWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent);
	~EditWidget();
	// IMessageWidget
	virtual QWidget *instance() { return this; }
	virtual bool isVisibleOnWindow() const;
	virtual IMessageWindow *messageWindow() const;
	// IMessageEditWidget
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
	virtual bool isRichTextEnabled() const;
	virtual void setRichTextEnabled(bool AEnabled);
	virtual bool isEditToolBarVisible() const;
	virtual void setEditToolBarVisible(bool AVisible);
	virtual ToolBarChanger *editToolBarChanger() const;
	virtual void contextMenuForEdit(const QPoint &APosition, Menu *AMenu);
	virtual void insertTextFragment(const QTextDocumentFragment &AFragment);
	virtual QTextDocumentFragment prepareTextFragment(const QTextDocumentFragment &AFragment) const;
signals:
	// IMessageEditWidget
	void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
	void messageAboutToBeSend();
	void messageReady();
	void editorCleared();
	void autoResizeChanged(bool AResize);
	void minimumLinesChanged(int ALines);
	void sendShortcutChanged(const QString &AShortcutId);
	void richTextEnableChanged(bool AEnabled);
	void contextMenuRequested(const QPoint &APosition, Menu *AMenu);
	// EditWidget
	void createDataRequest(QMimeData *ADestination) const;
	void canInsertDataRequest(const QMimeData *AData, bool &ACanInsert) const;
	void insertDataRequest(const QMimeData *AData, QTextDocument *ADocument) const;
	void contentsChanged(int APosition, int ARemoved, int AAdded) const;
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void appendMessageToBuffer();
	void showBufferedMessage();
	void showNextBufferedMessage();
	void showPrevBufferedMessage();
protected slots:
	void onUpdateEditToolBarMaxWidth();
	void onSendActionTriggered(bool);
	void onShortcutUpdated(const QString &AId);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onOptionsChanged(const OptionsNode &ANode);
	void onEditorCreateDataRequest(QMimeData *AData);
	void onEditorCanInsertDataRequest(const QMimeData *AData, bool &ACanInsert);
	void onEditorInsertDataRequest(const QMimeData *AData, QTextDocument *ADocument);
	void onEditorContentsChanged(int APosition, int ARemoved, int AAdded);
	void onEditorCustomContextMenuRequested(const QPoint &APosition);
private:
	Ui::EditWidgetClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	int FBufferPos;
	IMessageWindow *FWindow;
	QList<QString> FBuffer;
	QString FSendShortcutId;
	QKeySequence FSendShortcut;
	ToolBarChanger *FEditToolBar;
};

#endif // EDITWIDGET_H
