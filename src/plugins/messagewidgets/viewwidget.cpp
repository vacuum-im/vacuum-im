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
    Jid befour = FStreamJid;
    FStreamJid = AStreamJid;
    emit streamJidChanged(befour);
  }
}

void ViewWidget::setContactJid(const Jid &AContactJid)
{
  if (AContactJid != FContactJid)
  {
    Jid befour = FContactJid;
    FContactJid = AContactJid;
    emit contactJidChanged(befour);
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
    IMessageStyle *befour = FMessageStyle;
    FMessageStyle = AStyle;
    if (befour)
    {
      disconnect(befour->instance(),SIGNAL(contentAppended(QWidget *, const QString &, const IMessageContentOptions &)),
        this, SLOT(onContentAppended(QWidget *, const QString &, const IMessageContentOptions &)));
      disconnect(befour->instance(),SIGNAL(urlClicked(QWidget *, const QUrl &)),this,SLOT(onUrlClicked(QWidget *, const QUrl &)));
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
    emit messageStyleChanged(befour,AOptions);
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
  QTextDocument messageDoc;
  if (FMessageProcessor)
    FMessageProcessor->messageToText(&messageDoc,AMessage);
  else
    messageDoc.setPlainText(AMessage.body());

  appendHtml(getHtmlBody(messageDoc.toHtml()),AOptions);
}

void ViewWidget::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
  if (plugin)
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
}

QString ViewWidget::getHtmlBody(const QString &AHtml)
{
  QRegExp body("<body.*>(.*)</body>");
  body.setMinimal(false);
  return AHtml.indexOf(body)>=0 ? body.cap(1).trimmed() : AHtml;
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
