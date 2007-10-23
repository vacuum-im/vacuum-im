#include <QDebug>
#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>

ViewWidget::ViewWidget(IMessenger *AMessenger, const Jid &AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  FOptions = 0;
  FShowKind = ChatMessage;

  QToolBar *toolBar = new QToolBar(ui.wdtToolBar);
  toolBar->setIconSize(QSize(16,16));
  FToolBarLayout = new QVBoxLayout();
  FToolBarLayout->setMargin(0);
  FToolBarLayout->addWidget(toolBar);
  ui.wdtToolBar->setLayout(FToolBarLayout);
  FToolBarChanger = new ToolBarChanger(toolBar);

  setColorForJid(FStreamJid,Qt::red);
  setColorForJid(FContactJid,Qt::blue);
}

ViewWidget::~ViewWidget()
{

}

void ViewWidget::setShowKind(ShowKind AKind)
{
  FShowKind = AKind;
}

void ViewWidget::showMessage(const Message &AMessage)
{
  Jid authorJid = AMessage.from().isEmpty() ? FStreamJid : AMessage.from();
  QString authorNick = FJid2Nick.value(authorJid,authorJid.node());

  if (FShowKind == SingleMessage)
    document()->clear();

  QTextCursor cursor = document()->rootFrame()->lastCursorPosition();
  bool cursorVisible = textEdit()->viewport()->geometry().contains(textEdit()->cursorRect(cursor));

  QTextDocument messageDoc;
  FMessenger->messageToText(&messageDoc,AMessage,AMessage.defLang());

  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  tableFormat.setBorderStyle(QTextTableFormat::BorderStyle_None);
  QTextTable *table = cursor.insertTable(1,2,tableFormat);

  QTextCharFormat messageFormat;
  messageFormat.setFont(document()->defaultFont());

  if (FShowKind == ChatMessage)
  {
    QTextCharFormat timeFormat = messageFormat;
    timeFormat.setForeground(Qt::gray);
    QTextCharFormat nickFormat = messageFormat;
    nickFormat.setForeground(colorForJid(authorJid));

    if (FMessenger->checkOption(IMessenger::ShowDateTime))
      table->cellAt(0,0).lastCursorPosition().insertText(QString("[%1]").arg(AMessage.dateTime().toString("hh:mm")),timeFormat);
    table->cellAt(0,0).lastCursorPosition().insertText(QString("[%1]").arg(authorNick),nickFormat);
  }

  if (FMessenger->checkOption(IMessenger::ShowHTML))
    table->cellAt(0,1).lastCursorPosition().insertHtml(messageDoc.toHtml());
  else
    table->cellAt(0,1).lastCursorPosition().insertText(messageDoc.toPlainText().trimmed(),messageFormat);

  if (cursorVisible)
  {
    textEdit()->setTextCursor(document()->rootFrame()->lastCursorPosition());
    textEdit()->ensureCursorVisible();
  }

  emit messageShown(AMessage);
}

void ViewWidget::showCustomHtml(const QString &AHtml)
{
  QTextCursor cursor = document()->rootFrame()->lastCursorPosition();
  bool cursorVisible = textEdit()->viewport()->geometry().contains(textEdit()->cursorRect(cursor));
  
  cursor.insertHtml(AHtml);
  cursor.insertBlock();

  if (cursorVisible)
  {
    textEdit()->setTextCursor(document()->rootFrame()->lastCursorPosition());
    textEdit()->ensureCursorVisible();
  }

  emit customHtmlShown(AHtml);
}

void ViewWidget::setColorForJid(const Jid &AJid, const QColor &AColor)
{
  if (AColor.isValid())
    FJid2Color.insert(AJid,AColor);
  else
    FJid2Color.remove(AJid);
  colorForJidChanged(AJid,AColor);
}

void ViewWidget::setNickForJid(const Jid &AJid, const QString &ANick)
{
  if (!ANick.isNull())
    FJid2Nick.insert(AJid,ANick);
  else
    FJid2Nick.remove(ANick);
  nickForJidChanged(AJid,ANick);
}

void ViewWidget::setStreamJid(const Jid &AStreamJid)
{
  if (AStreamJid != FStreamJid)
  {
    Jid befour = FStreamJid;
    FStreamJid = AStreamJid;

    setColorForJid(FStreamJid,colorForJid(befour));
    setColorForJid(befour,QColor());
    setNickForJid(FStreamJid,nickForJid(befour));
    setNickForJid(befour,QString());

    emit streamJidChanged(befour);
  }
}

void ViewWidget::setContactJid(const Jid &AContactJid)
{
  if (AContactJid != FContactJid)
  {
    Jid befour = FContactJid;
    FContactJid = AContactJid;

    setColorForJid(FContactJid,colorForJid(befour));
    setColorForJid(befour,QColor());
    setNickForJid(FContactJid,nickForJid(befour));
    setNickForJid(befour,QString());

    emit contactJidChanged(AContactJid);
  }
}

