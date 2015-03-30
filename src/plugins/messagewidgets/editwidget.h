#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <interfaces/imessagewidgets.h>
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
	virtual bool sendMessage();
	virtual bool isSendEnabled() const;
	virtual void setSendEnabled(bool AEnabled);
	virtual bool isEditEnabled() const;
	virtual void setEditEnabled(bool AEnabled);
	virtual bool isAutoResize() const;
	virtual void setAutoResize(bool AAuto);
	virtual int minimumHeightLines() const;
	virtual void setMinimumHeightLines(int ALines);
	virtual bool isRichTextEnabled() const;
	virtual void setRichTextEnabled(bool AEnabled);
	virtual bool isEditToolBarVisible() const;
	virtual void setEditToolBarVisible(bool AVisible);
	virtual ToolBarChanger *editToolBarChanger() const;
	virtual QString sendShortcutId() const;
	virtual void setSendShortcutId(const QString &AShortcutId);
	virtual void contextMenuForEdit(const QPoint &APosition, Menu *AMenu);
	virtual void insertTextFragment(const QTextDocumentFragment &AFragment);
	virtual QTextDocumentFragment prepareTextFragment(const QTextDocumentFragment &AFragment);
signals:
	// IMessageEditWidget
	void messageSent();
	void sendEnableChanged(bool AEnabled);
	void editEnableChanged(bool AEnabled);
	void autoResizeChanged(bool AResize);
	void minimumHeightLinesChanged(int ALines);
	void richTextEnableChanged(bool AEnabled);
	void sendShortcutIdChanged(const QString &AShortcutId);
	void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
	void contextMenuRequested(const QPoint &APosition, Menu *AMenu);
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void appendMessageToBuffer();
	void showBufferedMessage();
	void showNextBufferedMessage();
	void showPrevBufferedMessage();
protected slots:
	void onUpdateEditToolBarVisibility();
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
	bool FSendEnabled;
	bool FEditEnabled;
	Action *FSendAction;
	IMessageWindow *FWindow;
	QList<QString> FBuffer;
	QString FSendShortcutId;
	QKeySequence FSendShortcut;
	ToolBarChanger *FEditToolBar;
};

#endif // EDITWIDGET_H
