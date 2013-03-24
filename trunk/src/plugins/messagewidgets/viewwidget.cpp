#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QTextDocumentFragment>

ViewWidget::ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAcceptDrops(true);

	QVBoxLayout *layout = new QVBoxLayout(ui.wdtViewer);
	layout->setMargin(0);

	FMessageStyle = NULL;
	FMessageProcessor = NULL;
	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FStyleWidget = NULL;

	initialize();
}

ViewWidget::~ViewWidget()
{

}

void ViewWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid before = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(before);
	}
}

void ViewWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid before = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(before);
	}
}

QWidget *ViewWidget::styleWidget() const
{
	return FStyleWidget;
}

IMessageStyle *ViewWidget::messageStyle() const
{
	return FMessageStyle;
}

void ViewWidget::setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions)
{
	if (FMessageStyle != AStyle)
	{
		IMessageStyle *before = FMessageStyle;
		FMessageStyle = AStyle;
		if (before)
		{
			disconnect(before->instance(),SIGNAL(contentAppended(QWidget *, const QString &, const IMessageContentOptions &)),
				this, SLOT(onContentAppended(QWidget *, const QString &, const IMessageContentOptions &)));
			disconnect(before->instance(),SIGNAL(urlClicked(QWidget *, const QUrl &)),this,SLOT(onUrlClicked(QWidget *, const QUrl &)));
			disconnect(FStyleWidget,SIGNAL(customContextMenuRequested(const QPoint &)),this,SLOT(onCustomContextMenuRequested(const QPoint &)));
			ui.wdtViewer->layout()->removeWidget(FStyleWidget);
			FStyleWidget->deleteLater();
			FStyleWidget = NULL;
		}
		if (FMessageStyle)
		{
			FStyleWidget = FMessageStyle->createWidget(AOptions,ui.wdtViewer);
			FStyleWidget->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(FStyleWidget,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onCustomContextMenuRequested(const QPoint &)));
			connect(FMessageStyle->instance(),SIGNAL(contentAppended(QWidget *, const QString &, const IMessageContentOptions &)),
				SLOT(onContentAppended(QWidget *, const QString &, const IMessageContentOptions &)));
			connect(FMessageStyle->instance(),SIGNAL(urlClicked(QWidget *, const QUrl &)),SLOT(onUrlClicked(QWidget *, const QUrl &)));
			ui.wdtViewer->layout()->addWidget(FStyleWidget);
		}
		emit messageStyleChanged(before,AOptions);
	}
}

void ViewWidget::appendHtml(const QString &AHtml, const IMessageContentOptions &AOptions)
{
	if (FMessageStyle)
		FMessageStyle->appendContent(FStyleWidget,AHtml,AOptions);
}

void ViewWidget::appendText(const QString &AText, const IMessageContentOptions &AOptions)
{
	Message message;
	message.setBody(AText);
	appendMessage(message,AOptions);
}

void ViewWidget::appendMessage(const Message &AMessage, const IMessageContentOptions &AOptions)
{
	QTextDocument doc;
	if (FMessageProcessor)
		FMessageProcessor->messageToText(&doc,AMessage);
	else
		doc.setPlainText(AMessage.body());

	// "/me" command
	IMessageContentOptions options = AOptions;
	if (AOptions.kind==IMessageContentOptions::KindMessage && !AOptions.senderName.isEmpty())
	{
		QTextCursor cursor(&doc);
		cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,4);
		if (cursor.selectedText() == "/me ")
		{
			options.kind = IMessageContentOptions::KindMeCommand;
			cursor.removeSelectedText();
		}
	}

	appendHtml(TextManager::getDocumentBody(doc),options);
}

void ViewWidget::contextMenuForView(const QPoint &APosition, Menu *AMenu)
{
	emit viewContextMenu(APosition, AMenu);
}

QTextDocumentFragment ViewWidget::selection() const
{
	return FMessageStyle!=NULL ? FMessageStyle->selection(FStyleWidget) : QTextDocumentFragment();
}

QTextCharFormat ViewWidget::textFormatAt(const QPoint &APosition) const
{
	if (FMessageStyle && FStyleWidget)
		return FMessageStyle->textFormatAt(FStyleWidget,FStyleWidget->mapFromGlobal(mapToGlobal(APosition)));
	return QTextCharFormat();
}

QTextDocumentFragment ViewWidget::textFragmentAt(const QPoint &APosition) const
{
	if (FMessageStyle && FStyleWidget)
		return FMessageStyle->textFragmentAt(FStyleWidget,FStyleWidget->mapFromGlobal(mapToGlobal(APosition)));
	return QTextDocumentFragment();
}

void ViewWidget::initialize()
{
	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
}

void ViewWidget::dropEvent(QDropEvent *AEvent)
{
	Menu *dropMenu = new Menu(this);

	bool accepted = false;
	foreach(IViewDropHandler *handler, FActiveDropHandlers)
		if (handler->viewDropAction(this, AEvent, dropMenu))
			accepted = true;

	if (accepted && !dropMenu->isEmpty())
	{
		if (dropMenu->exec(mapToGlobal(AEvent->pos())))
			AEvent->acceptProposedAction();
		else
			AEvent->ignore();
	}
	else
	{
		AEvent->ignore();
	}
	delete dropMenu;
}

void ViewWidget::dragEnterEvent(QDragEnterEvent *AEvent)
{
	FActiveDropHandlers.clear();
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDragEnter(this, AEvent))
			FActiveDropHandlers.append(handler);

	if (!FActiveDropHandlers.isEmpty())
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void ViewWidget::dragMoveEvent(QDragMoveEvent *AEvent)
{
	bool accepted = false;
	foreach(IViewDropHandler *handler, FActiveDropHandlers)
		if (handler->viewDragMove(this, AEvent))
			accepted = true;

	if (accepted)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void ViewWidget::dragLeaveEvent(QDragLeaveEvent *AEvent)
{
	foreach(IViewDropHandler *handler, FActiveDropHandlers)
		handler->viewDragLeave(this, AEvent);
}

void ViewWidget::onContentAppended(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions)
{
	if (AWidget == FStyleWidget)
		emit contentAppended(AHtml,AOptions);
}

void ViewWidget::onUrlClicked(QWidget *AWidget, const QUrl &AUrl)
{
	if (AWidget == FStyleWidget)
		emit urlClicked(AUrl);
}

void ViewWidget::onCustomContextMenuRequested(const QPoint &APosition)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose, true);

	contextMenuForView(APosition, menu);

	if (!menu->isEmpty())
		menu->popup(FStyleWidget->mapToGlobal(APosition));
	else
		delete menu;
}
