#include "editwidget.h"

#include <QPair>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#define MAX_BUFFERED_MESSAGES     10

EditWidget::EditWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	ui.medEditor->setAcceptRichText(true);
	ui.medEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

	FMessageWidgets = AMessageWidgets;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FBufferPos = -1;
	FFormatEnabled = false;

	FSendShortcut = new QShortcut(ui.medEditor);
	FSendShortcut->setContext(Qt::WidgetShortcut);
	connect(FSendShortcut,SIGNAL(activated()),SLOT(onShortcutActivated()));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.tlbSend,MNI_MESSAGEWIDGETS_SEND);
	connect(ui.tlbSend,SIGNAL(clicked(bool)),SLOT(onSendButtonCliked(bool)));

	ui.medEditor->installEventFilter(this);
	connect(ui.medEditor->document(),SIGNAL(contentsChange(int,int,int)),SLOT(onContentsChanged(int,int,int)));

	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORAUTORESIZE));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORMINIMUMLINES));
	onOptionsChanged(Options::node(OPV_MESSAGES_EDITORSENDKEY));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

EditWidget::~EditWidget()
{

}

const Jid &EditWidget::streamJid() const
{
	return FStreamJid;
}

void EditWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid before = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(before);
	}
}

const Jid & EditWidget::contactJid() const
{
	return FContactJid;
}

void EditWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid before = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(before);
	}
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

QKeySequence EditWidget::sendKey() const
{
	return FSendShortcut->key();
}

void EditWidget::setSendKey(const QKeySequence &AKey)
{
	FSendShortcut->setKey(AKey);
	emit sendKeyChanged(AKey);
}

bool EditWidget::sendButtonVisible() const
{
	return ui.tlbSend->isVisible();
}

void EditWidget::setSendButtonVisible(bool AVisible)
{
	ui.tlbSend->setVisible(AVisible);
}

bool EditWidget::textFormatEnabled() const
{
	return FFormatEnabled;
}

void EditWidget::setTextFormatEnabled(bool AEnabled)
{
	FFormatEnabled = AEnabled;
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	bool hooked = false;
	if (AWatched==ui.medEditor && AEvent->type()==QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		emit keyEventReceived(keyEvent,hooked);

		if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key()==Qt::Key_Up)
		{
			hooked = true;
			showNextBufferedMessage();
		}
		else if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key() == Qt::Key_Down)
		{
			hooked = true;
			showPrevBufferedMessage();
		}
	}
	else if (AWatched==ui.medEditor && AEvent->type()==QEvent::ShortcutOverride)
	{
		hooked = true;
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

void EditWidget::onShortcutActivated()
{
	QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
	if (shortcut == FSendShortcut)
	{
		sendMessage();
	}
}

void EditWidget::onSendButtonCliked(bool)
{
	sendMessage();
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
	else if (ANode.path() == OPV_MESSAGES_EDITORSENDKEY)
	{
		setSendKey(ANode.value().value<QKeySequence>());
	}
}

void EditWidget::onContentsChanged(int APosition, int ARemoved, int AAdded)
{
	Q_UNUSED(ARemoved);
	if (!FFormatEnabled && AAdded>0)
	{
		QTextCharFormat emptyFormat;
		QList< QPair<int,int> > formats;
		QTextBlock block = ui.medEditor->document()->findBlock(APosition);
		while (block.isValid() && block.position()<=APosition+AAdded)
		{
			for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
			{
				QTextCharFormat textFormat = it.fragment().charFormat();
				if (!textFormat.isImageFormat() && textFormat!=emptyFormat)
					formats.append(qMakePair(it.fragment().position(),it.fragment().length()));
			}
			block = block.next();
		}

		if (!formats.isEmpty())
		{
			QTextCursor cursor(ui.medEditor->document());
			cursor.beginEditBlock();
			for (int i=0; i<formats.count(); i++)
			{
				const QPair<int,int> &format = formats.at(i);
				cursor.setPosition(format.first);
				cursor.setPosition(format.first + format.second, QTextCursor::KeepAnchor);
				cursor.setCharFormat(emptyFormat);
			}
			cursor.endEditBlock();
		}
	}
}
