#include "editwidget.h"

#include <QKeyEvent>

#define MAX_BUFFERED_MESSAGES     10

EditWidget::EditWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.medEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;
	
	FBufferPos = -1;
	setRichTextEnabled(false);

	QToolBar *toolBar = new QToolBar;
	toolBar->setMovable(false);
	toolBar->setFloatable(false);
	toolBar->setIconSize(QSize(16,16));
	toolBar->layout()->setMargin(0);
	toolBar->setStyleSheet("QToolBar { border: none; }");
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

	FEditToolBar = new ToolBarChanger(toolBar);
	FEditToolBar->setSeparatorsVisible(false);
	FEditToolBar->toolBar()->installEventFilter(this);

	ui.wdtSendToolBar->setLayout(new QHBoxLayout);
	ui.wdtSendToolBar->layout()->setMargin(0);
	ui.wdtSendToolBar->layout()->addWidget(toolBar);

	Action *sendAction = new Action(toolBar);
	sendAction->setToolTip(tr("Send"));
	sendAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MESSAGEWIDGETS_SEND);
	connect(sendAction,SIGNAL(triggered(bool)),SLOT(onSendActionTriggered(bool)));
	FEditToolBar->insertAction(sendAction,TBG_MWEWTB_SENDMESSAGE);

	ui.medEditor->installEventFilter(this);
	ui.medEditor->setContextMenuPolicy(Qt::CustomContextMenu);
	Shortcuts::insertWidgetShortcut(SCT_MESSAGEWINDOWS_EDITNEXTMESSAGE,ui.medEditor);
	Shortcuts::insertWidgetShortcut(SCT_MESSAGEWINDOWS_EDITPREVMESSAGE,ui.medEditor);
	connect(ui.medEditor,SIGNAL(createDataRequest(QMimeData *)),SLOT(onEditorCreateDataRequest(QMimeData *)));
	connect(ui.medEditor,SIGNAL(canInsertDataRequest(const QMimeData *, bool &)),
		SLOT(onEditorCanInsertDataRequest(const QMimeData *, bool &)));
	connect(ui.medEditor,SIGNAL(insertDataRequest(const QMimeData *, QTextDocument *)),
		SLOT(onEditorInsertDataRequest(const QMimeData *, QTextDocument *)));
	connect(ui.medEditor->document(),SIGNAL(contentsChange(int,int,int)),SLOT(onEditorContentsChanged(int,int,int)));
	connect(ui.medEditor,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onEditorCustomContextMenuRequested(const QPoint &)));

	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORAUTORESIZE));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORMINIMUMLINES));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(Shortcuts::instance(),SIGNAL(shortcutUpdated(const QString &)),SLOT(onShortcutUpdated(const QString &)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));
}

EditWidget::~EditWidget()
{

}

bool EditWidget::isVisibleOnWindow() const
{
	return isVisibleTo(FWindow->instance());
}

IMessageWindow *EditWidget::messageWindow() const
{
	return FWindow;
}

QTextEdit *EditWidget::textEdit() const
{
	return ui.medEditor;
}

QTextDocument *EditWidget::document() const
{
	return ui.medEditor->document();
}

void EditWidget::sendMessage()
{
	emit messageAboutToBeSend();
	appendMessageToBuffer();
	emit messageReady();
}

void EditWidget::clearEditor()
{
	ui.medEditor->clear();
	emit editorCleared();
}

bool EditWidget::autoResize() const
{
	return ui.medEditor->autoResize();
}

void EditWidget::setAutoResize(bool AResize)
{
	ui.medEditor->setAutoResize(AResize);
	emit autoResizeChanged(ui.medEditor->autoResize());
}

int EditWidget::minimumLines() const
{
	return ui.medEditor->minimumLines();
}

void EditWidget::setMinimumLines(int ALines)
{
	ui.medEditor->setMinimumLines(ALines);
	emit minimumLinesChanged(ui.medEditor->minimumLines());
}

QString EditWidget::sendShortcut() const
{
	return FSendShortcutId;
}

void EditWidget::setSendShortcut(const QString &AShortcutId)
{
	if (FSendShortcutId != AShortcutId)
	{
		if (!FSendShortcutId.isEmpty())
			Shortcuts::removeWidgetShortcut(FSendShortcutId,ui.medEditor);
		FSendShortcutId = AShortcutId;
		if (!FSendShortcutId.isEmpty())
			Shortcuts::insertWidgetShortcut(FSendShortcutId,ui.medEditor);
		onShortcutUpdated(FSendShortcutId);
		emit sendShortcutChanged(FSendShortcutId);
	}
}

bool EditWidget::isRichTextEnabled() const
{
	return ui.medEditor->acceptRichText();
}

void EditWidget::setRichTextEnabled(bool AEnabled)
{
	if (isRichTextEnabled() != AEnabled)
	{
		ui.medEditor->setAcceptRichText(AEnabled);
		richTextEnableChanged(AEnabled);
	}
}

bool EditWidget::isEditToolBarVisible() const
{
	return FEditToolBar->toolBar()->isVisible();
}

void EditWidget::setEditToolBarVisible(bool AVisible)
{
	FEditToolBar->toolBar()->setVisible(AVisible);
}

ToolBarChanger *EditWidget::editToolBarChanger() const
{
	return FEditToolBar;
}

void EditWidget::contextMenuForEdit(const QPoint &APosition, Menu *AMenu)
{
	QMenu *stdMenu = ui.medEditor->createStandardContextMenu(APosition);
	Menu::copyStandardMenu(AMenu,stdMenu,AG_EWCM_MESSAGEWIDGETS_DEFAULT);
	connect(AMenu,SIGNAL(destroyed(QObject *)),stdMenu,SLOT(deleteLater()));
	emit contextMenuRequested(APosition,AMenu);
}

void EditWidget::insertTextFragment(const QTextDocumentFragment &AFragment)
{
	if (!AFragment.isEmpty())
	{
		if (isRichTextEnabled())
			ui.medEditor->textCursor().insertFragment(prepareTextFragment(AFragment));
		else
			ui.medEditor->textCursor().insertText(prepareTextFragment(AFragment).toPlainText());
	}
}

QTextDocumentFragment EditWidget::prepareTextFragment(const QTextDocumentFragment &AFragment) const
{
	QTextDocumentFragment fragment;
	if (!AFragment.isEmpty())
	{
		QMimeData data;
		data.setHtml(AFragment.toHtml());

		QTextDocument doc;
		emit insertDataRequest(&data,&doc);

		if (isRichTextEnabled())
			fragment = QTextDocumentFragment::fromHtml(doc.toHtml());
		else
			fragment = QTextDocumentFragment::fromPlainText(doc.toPlainText());
	}
	return fragment;
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	bool hooked = false;
	if (AWatched == ui.medEditor)
	{
		if (AEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
			if (FSendShortcut[0] == keyEvent->key()+keyEvent->modifiers())
			{
				hooked = true;
				onShortcutActivated(FSendShortcutId,ui.medEditor);
			}
			else
			{
				emit keyEventReceived(keyEvent,hooked);
			}
		}
		else if (AEvent->type() == QEvent::ShortcutOverride)
		{
			hooked = true;
		}
	}
	else if (AWatched == FEditToolBar->toolBar())
	{
		if (AEvent->type() == QEvent::LayoutRequest)
			QTimer::singleShot(0,this,SLOT(onUpdateEditToolBarMaxWidth()));
	}
	return hooked || QWidget::eventFilter(AWatched,AEvent);
}

void EditWidget::appendMessageToBuffer()
{
	QString message = ui.medEditor->toPlainText();
	if (!message.isEmpty())
	{
		FBufferPos = -1;
		int index = FBuffer.indexOf(message);
		if (index >= 0)
			FBuffer.removeAt(index);
		FBuffer.prepend(message);
		if (FBuffer.count() > MAX_BUFFERED_MESSAGES)
			FBuffer.removeLast();
	}
}

void EditWidget::showBufferedMessage()
{
	ui.medEditor->setPlainText(FBuffer.value(FBufferPos));
	ui.medEditor->moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
}

void EditWidget::showNextBufferedMessage()
{
	if (FBufferPos < FBuffer.count()-1)
	{
		if (FBufferPos<0 && !ui.medEditor->toPlainText().isEmpty())
		{
			appendMessageToBuffer();
			FBufferPos++;
		}
		FBufferPos++;
		showBufferedMessage();
	}
}

void EditWidget::showPrevBufferedMessage()
{
	if (FBufferPos > 0)
	{
		FBufferPos--;
		showBufferedMessage();
	}
}

void EditWidget::onUpdateEditToolBarMaxWidth()
{
	int widgetWidth = 0;
	int visibleItemsCount = 0;
	for (int itemIndex=0; itemIndex<FEditToolBar->toolBar()->layout()->count(); itemIndex++)
	{
		QWidget *widget = FEditToolBar->toolBar()->layout()->itemAt(itemIndex)->widget();
		if (widget && widget->isVisible())
		{
			visibleItemsCount++;
			widgetWidth = widget->sizeHint().width();
		}
	}
	FEditToolBar->toolBar()->setMaximumWidth(visibleItemsCount==1 ? widgetWidth : QWIDGETSIZE_MAX);
}

void EditWidget::onSendActionTriggered(bool)
{
	sendMessage();
}

void EditWidget::onShortcutUpdated(const QString &AId)
{
	if (AId == FSendShortcutId)
		FSendShortcut = Shortcuts::shortcutDescriptor(AId).activeKey;
}

void EditWidget::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==FSendShortcutId && AWidget==ui.medEditor)
	{
		sendMessage();
	}
	else if (AId==SCT_MESSAGEWINDOWS_EDITNEXTMESSAGE && AWidget==ui.medEditor)
	{
		showPrevBufferedMessage();
	}
	else if (AId==SCT_MESSAGEWINDOWS_EDITPREVMESSAGE && AWidget==ui.medEditor)
	{
		showNextBufferedMessage();
	}
}

void EditWidget::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_EDITORAUTORESIZE)
	{
		setAutoResize(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_MESSAGES_EDITORMINIMUMLINES)
	{
		setMinimumLines(ANode.value().toInt());
	}
}

void EditWidget::onEditorCreateDataRequest(QMimeData *AData)
{
	emit createDataRequest(AData);
}

void EditWidget::onEditorCanInsertDataRequest(const QMimeData *AData, bool &ACanInsert)
{
	emit canInsertDataRequest(AData,ACanInsert);
}

void EditWidget::onEditorInsertDataRequest(const QMimeData *AData, QTextDocument *ADocument)
{
	emit insertDataRequest(AData,ADocument);
}

void EditWidget::onEditorContentsChanged(int APosition, int ARemoved, int AAdded)
{
	emit contentsChanged(APosition,ARemoved,AAdded);
}

void EditWidget::onEditorCustomContextMenuRequested(const QPoint &APosition)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose, true);

	contextMenuForEdit(APosition,menu);

	if (!menu->isEmpty())
		menu->popup(ui.medEditor->mapToGlobal(APosition));
	else
		delete menu;
}
