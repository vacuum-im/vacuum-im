#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include "ui_viewwidget.h"

class ViewWidget : 
  public QWidget,
  public IViewWidget
{
  Q_OBJECT;
  Q_INTERFACES(IViewWidget);
public:
  ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
  ~ViewWidget();
  virtual QWidget *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual QWidget *styleWidget() const;
  virtual IMessageStyle *messageStyle() const;
  virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions);
  virtual void appendHtml(const QString &AHtml, const IMessageContentOptions &AOptions);
  virtual void appendText(const QString &AText, const IMessageContentOptions &AOptions);
  virtual void appendMessage(const Message &AMessage, const IMessageContentOptions &AOptions);
signals:
  void streamJidChanged(const Jid &ABefour);
  void contactJidChanged(const Jid &ABefour);
  void messageStyleChanged(IMessageStyle *ABefour, const IMessageStyleOptions &AOptions);
  void contentAppended(const QString &AMessage, const IMessageContentOptions &AOptions);
  void urlClicked(const QUrl &AUrl) const;
protected:
  void initialize();
  QString getHtmlBody(const QString &AHtml);
  bool processMeCommand(QTextDocument *ADocument, const IMessageContentOptions &AOptions);
protected slots:
  void onContentAppended(QWidget *AWidget, const QString &AMessage, const IMessageContentOptions &AOptions);
  void onUrlClicked(QWidget *AWidget, const QUrl &AUrl);
private:
  Ui::ViewWidgetClass ui;
private:
  IMessageStyle *FMessageStyle;
  IMessageWidgets *FMessageWidgets;
  IMessageProcessor *FMessageProcessor;
private:
  Jid FStreamJid;
  Jid FContactJid;
  QWidget *FStyleWidget;
};

#endif // VIEWWIDGET_H
