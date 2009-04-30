#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QWebView>
#include "../../interfaces/imessagewidgets.h"
#include "../../interfaces/imessageprocessor.h"
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
  virtual QWebView *webBrowser() const;
  virtual void setHtml(const QString &AHtml);
  virtual void setMessage(const Message &AMessage);
  virtual IMessageStyle *messageStyle() const;
  virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyle::StyleOptions &AOptions);
  virtual const IMessageStyles::ContentSettings &contentSettings() const;
  virtual void setContentSettings(const IMessageStyles::ContentSettings &ASettings);
  virtual void appendHtml(const QString &AHtml, const IMessageStyle::ContentOptions &AOptions);
  virtual void appendText(const QString &AText, const IMessageStyle::ContentOptions &AOptions);
  virtual void appendMessage(const Message &AMessage, const IMessageStyle::ContentOptions &AOptions);
signals:
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void messageStyleChanged(IMessageStyle *ABefour, const IMessageStyle::StyleOptions &AOptions);
  virtual void contentSettingsChanged(const IMessageStyles::ContentSettings &ASettings);
  virtual void contentAppended(const QString &AMessage, const IMessageStyle::ContentOptions &AOptions);
signals:
  void linkClicked(const QUrl &AUrl);
protected:
  void initialize();
  QString getHtmlBody(const QString &AHtml);
  bool processMeCommand(QTextDocument *ADocument, const IMessageStyle::ContentOptions &AOptions);
protected slots:
  void onContentAppended(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions);
private:
  Ui::ViewWidgetClass ui;
private:
  IMessageStyle *FMessageStyle;
  IMessageWidgets *FMessageWidgets;
  IMessageProcessor *FMessageProcessor;
private:
  Jid FStreamJid;
  Jid FContactJid;
  IMessageStyles::ContentSettings FContentSettings;
};

#endif // VIEWWIDGET_H
