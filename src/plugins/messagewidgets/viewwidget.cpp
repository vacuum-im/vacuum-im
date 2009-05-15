#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QVBoxLayout>

ViewWidget::ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

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

  if (!AOptions.senderName.isEmpty() && processMeCommand(&messageDoc,AOptions))
  {
    IMessageContentOptions options = AOptions;
    options.senderName = QString::null;
    appendHtml(getHtmlBody(messageDoc.toHtml()),options);
  }
  else
    appendHtml(getHtmlBody(messageDoc.toHtml()),AOptions);
}

void ViewWidget::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->getPlugins("IMessageProcessor").value(0,NULL);
  if (plugin)
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
}

QString ViewWidget::getHtmlBody(const QString &AHtml)
{
  QRegExp bodyStart("<body.*>");
  QRegExp bodyEnd("</body>");
  int start = AHtml.indexOf(bodyStart);
  int end = AHtml.lastIndexOf(bodyEnd);
  if (start >=0 && end > start)
  {
    start = AHtml.indexOf(">",start)+1;
    return AHtml.mid(start,end-start);
  }
  return AHtml;
}

bool ViewWidget::processMeCommand(QTextDocument *ADocument, const IMessageContentOptions &AOptions)
{
  QRegExp regexp("^/me\\s");
  for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
  {
    cursor.insertHtml("<i>* "+AOptions.senderName+"&nbsp;</i>");
    return true;
  }
  return false;
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