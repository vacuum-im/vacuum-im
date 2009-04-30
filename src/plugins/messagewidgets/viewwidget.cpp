#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QResizeEvent>

ViewWidget::ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FMessageStyle = NULL;
  FMessageProcessor = NULL;
  FMessageWidgets = AMessageWidgets;

  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  ui.mvrViewer->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  connect(ui.mvrViewer->page(),SIGNAL(linkClicked(const QUrl &)),SIGNAL(linkClicked(const QUrl &)));

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

QWebView *ViewWidget::webBrowser() const
{
  return ui.mvrViewer;
}

void ViewWidget::setHtml(const QString &AHtml)
{
  ui.mvrViewer->setHtml(AHtml);
}

void ViewWidget::setMessage(const Message &AMessage)
{
  QTextDocument messageDoc;
  if (FMessageProcessor)
    FMessageProcessor->messageToText(&messageDoc,AMessage);
  else
    messageDoc.setPlainText(AMessage.body());

  setHtml(messageDoc.toHtml());
}

IMessageStyle *ViewWidget::messageStyle() const
{
  return FMessageStyle;
}

void ViewWidget::setMessageStyle(IMessageStyle *AStyle, const IMessageStyle::StyleOptions &AOptions)
{
  if (FMessageStyle != AStyle)
  {
    IMessageStyle *befour = FMessageStyle;
    FMessageStyle = AStyle;
    if (befour)
    {
      disconnect(befour->instance(),SIGNAL(contentAppended(QWebView *, const QString &, const IMessageStyle::ContentOptions &)),
        this, SLOT(onContentAppended(QWebView *, const QString &, const IMessageStyle::ContentOptions &)));
    }
    if (FMessageStyle)
    {
      FMessageStyle->setStyle(ui.mvrViewer,AOptions);
      connect(FMessageStyle->instance(),SIGNAL(contentAppended(QWebView *, const QString &, const IMessageStyle::ContentOptions &)),
        SLOT(onContentAppended(QWebView *, const QString &, const IMessageStyle::ContentOptions &)));
    }
    emit messageStyleChanged(befour,AOptions);
  }
}

const IMessageStyles::ContentSettings &ViewWidget::contentSettings() const
{
  return FContentSettings;
}

void ViewWidget::setContentSettings(const IMessageStyles::ContentSettings &ASettings)
{
  FContentSettings = ASettings;
  emit contentSettingsChanged(FContentSettings);
}

void ViewWidget::appendHtml(const QString &AHtml, const IMessageStyle::ContentOptions &AOptions)
{
  if (FMessageStyle)
    FMessageStyle->appendContent(ui.mvrViewer,AHtml,AOptions);
}

void ViewWidget::appendText(const QString &AText, const IMessageStyle::ContentOptions &AOptions)
{
  Message message;
  message.setBody(AText);
  appendMessage(message,AOptions);
}

void ViewWidget::appendMessage(const Message &AMessage, const IMessageStyle::ContentOptions &AOptions)
{
  QTextDocument messageDoc;
  if (FMessageProcessor)
    FMessageProcessor->messageToText(&messageDoc,AMessage);
  else
    messageDoc.setPlainText(AMessage.body());

  if (!AOptions.senderName.isEmpty() && processMeCommand(&messageDoc,AOptions))
  {
    IMessageStyle::ContentOptions options = AOptions;
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

bool ViewWidget::processMeCommand(QTextDocument *ADocument, const IMessageStyle::ContentOptions &AOptions)
{
  QRegExp regexp("^/me\\s");
  for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
  {
    cursor.insertHtml("<i>* "+AOptions.senderName+"&nbsp;</i>");
    return true;
  }
  return false;
}

void ViewWidget::onContentAppended(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions)
{
  if (AView == ui.mvrViewer)
    emit contentAppended(AMessage,AOptions);
}
