#include <QDebug>
#include "viewwidget.h"

#include <QTextFrame>
#include <QTextTable>
#include <QScrollBar>
#include <QResizeEvent>

ViewWidget::ViewWidget(IMessenger *AMessenger, const Jid &AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  FOptions = 0;
  FSetScrollToMax = false;
  FShowKind = ChatMessage;

  ui.tedViewer->installEventFilter(this);

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
  if (FShowKind == ChatMessage)
  {
    QTextCursor cursor = document()->rootFrame()->lastCursorPosition();
    bool scrollAtEnd = textBrowser()->verticalScrollBar()->sliderPosition() == textBrowser()->verticalScrollBar()->maximum();

    Jid authorJid = AMessage.from().isEmpty() ? FStreamJid : AMessage.from();
    QString authorNick = FJid2Nick.value(authorJid,authorJid.node());

    QTextCharFormat messageFormat;
    QTextCharFormat timeFormat;
    timeFormat.setForeground(Qt::gray);
    QTextCharFormat nickFormat;
    nickFormat.setForeground(colorForJid(authorJid));

    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setBorderStyle(QTextTableFormat::BorderStyle_None);
    QTextTable *table = cursor.insertTable(1,2,tableFormat);

    if (FMessenger->checkOption(IMessenger::ShowDateTime))
      table->cellAt(0,0).lastCursorPosition().insertText(QString("[%1]").arg(AMessage.dateTime().toString("hh:mm")),timeFormat);
    table->cellAt(0,0).lastCursorPosition().insertText(QString("[%1]").arg(authorNick),nickFormat);

    QTextDocument messageDoc;
    messageDoc.setDefaultFont(document()->defaultFont());
    FMessenger->messageToText(&messageDoc,AMessage);

    if (FMessenger->checkOption(IMessenger::ShowHTML))
      table->cellAt(0,1).lastCursorPosition().insertHtml(getHtmlBody(messageDoc.toHtml()));
    else
      table->cellAt(0,1).lastCursorPosition().insertText(messageDoc.toPlainText().trimmed(),messageFormat);

    if (scrollAtEnd)
      textBrowser()->verticalScrollBar()->setSliderPosition(textBrowser()->verticalScrollBar()->maximum());
  }
  else if (FShowKind == SingleMessage)
  {
    QTextDocument messageDoc;
    messageDoc.setDefaultFont(document()->defaultFont());
    FMessenger->messageToText(&messageDoc,AMessage);

    if (FMessenger->checkOption(IMessenger::ShowHTML))
      document()->setHtml(getHtmlBody(messageDoc.toHtml()));
    else
      document()->setPlainText(messageDoc.toPlainText().trimmed());
  }

  emit messageShown(AMessage);
}

void ViewWidget::showCustomHtml(const QString &AHtml)
{
  QTextCursor cursor = document()->rootFrame()->lastCursorPosition();
  bool scrollAtEnd = textBrowser()->verticalScrollBar()->sliderPosition() == textBrowser()->verticalScrollBar()->maximum();
  
  cursor.insertHtml(AHtml);
  cursor.insertBlock();

  if (scrollAtEnd)
    textBrowser()->verticalScrollBar()->setSliderPosition(textBrowser()->verticalScrollBar()->maximum());

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

bool ViewWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  if (AWatched == ui.tedViewer && AEvent->type() == QEvent::Resize)
  {
    QScrollBar *scrollBar = ui.tedViewer->verticalScrollBar();
    FSetScrollToMax = FSetScrollToMax || (scrollBar->sliderPosition() == scrollBar->maximum());
  }
  else if (AWatched == ui.tedViewer && AEvent->type() == QEvent::Paint)
  {
    if (FSetScrollToMax)
    {
      QScrollBar *scrollBar = ui.tedViewer->verticalScrollBar();
      scrollBar->setSliderPosition(scrollBar->maximum());
      FSetScrollToMax = false;
    }
  }

  return QWidget::eventFilter(AWatched,AEvent);
}

