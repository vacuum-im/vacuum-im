#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QTextDocumentFragment>

ViewWidget::ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
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
			ui.wdtViewer->layout()->removeWidget(FStyleWidget);
			FStyleWidget->deleteLater();
			FStyleWidget = NULL;
		}
		if (FMessageStyle)
		{
			FStyleWidget = FMessageStyle->createWidget(AOptions,ui.wdtViewer);
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
	if (AOptions.kind==IMessageContentOptions::Message && !AOptions.senderName.isEmpty())
	{
		QTextCursor cursor(&doc);
		cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,4);
		if (cursor.selectedText() == "/me ")
		{
			options.kind = IMessageContentOptions::MeCommand;
			cursor.removeSelectedText();
		}
	}

	appendHtml(getDocumentBody(doc),options);
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
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDropAction(this, AEvent, dropMenu))
			accepted = true;

	QAction *action= (AEvent->mouseButtons() & Qt::RightButton)>0 || dropMenu->defaultAction()==NULL ? dropMenu->exec(mapToGlobal(AEvent->pos())) : dropMenu->defaultAction();
	if (accepted && action)
	{
		action->trigger();
		AEvent->acceptProposedAction();
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
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		if (handler->viewDragMove(this, AEvent))
			accepted = true;

	if (accepted)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void ViewWidget::dragLeaveEvent(QDragLeaveEvent *AEvent)
{
	foreach(IViewDropHandler *handler, FMessageWidgets->viewDropHandlers())
		handler->viewDragLeave(this, AEvent);
}

void ViewWidget::onContentAppended(QWidget *AWidget, const QString &AMessage, const IMessageContentOptions &AOptions)
{
	if (AWidget == FStyleWidget)
		emit contentAppended(AMessage,AOptions);
}

void ViewWidget::onUrlClicked(QWidget *AWidget, const QUrl &AUrl)
{
	if (AWidget == FStyleWidget)
		emit urlClicked(AUrl);
}
