#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QWidget>
#include "../../definations/messagedataroles.h"
#include "../../interfaces/imessenger.h"
#include "../../utils/message.h"
#include "ui_viewwidget.h"

class ViewWidget : 
  public IViewWidget
{
  Q_OBJECT;
  Q_INTERFACES(IViewWidget);
public:
  ViewWidget(IMessenger *AMessenger, const Jid &AStreamJid, const Jid &AContactJid);
  ~ViewWidget();
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual QTextBrowser *textBrowser() const { return ui.tedViewer; }
  virtual QTextDocument *document() const { return ui.tedViewer->document(); }
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
  virtual ShowKind showKind() const { return FShowKind; }
  virtual void setShowKind(ShowKind AKind);
  virtual void showMessage(const Message &AMessage);
  virtual void showCustomHtml(const QString &AHtml);
  virtual QColor colorForJid(const Jid &AJid) const { return FJid2Color.value(AJid); }
  virtual void setColorForJid(const Jid &AJid, const QColor &AColor);
  virtual QString nickForJid(const Jid &AJid) const { return FJid2Nick.value(AJid); }
  virtual void setNickForJid(const Jid &AJid, const QString &ANick);
signals:
  virtual void messageShown(const Message &AMessage);
  virtual void customHtmlShown(const QString &AHtml);
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void colorForJidChanged(const Jid &AJid, const QColor &AColor);
  virtual void nickForJidChanged(const Jid &AJid, const QString &ANick);
protected:
  QString getHtmlBody(const QString &AHtml);
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
  Ui::ViewWidgetClass ui;
  QVBoxLayout *FToolBarLayout;
  ToolBarChanger *FToolBarChanger;
private:
  IMessenger *FMessenger;
private:
  Jid FStreamJid;
  Jid FContactJid;
  int FOptions;
  bool FSetScrollToMax;
  ShowKind FShowKind;
  QHash<Jid,QString> FJid2Nick;
  QHash<Jid,QColor> FJid2Color;
};

#endif // VIEWWIDGET_H
