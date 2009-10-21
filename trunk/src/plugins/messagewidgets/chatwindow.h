#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <definations/messagedataroles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/isettings.h>
#include "ui_chatwindow.h"

class ChatWindow : 
  public QMainWindow,
  public IChatWindow
{
  Q_OBJECT;
  Q_INTERFACES(IChatWindow ITabWidget);
public:
  ChatWindow(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
  virtual ~ChatWindow();
  virtual QMainWindow *instance() { return this; }
  //ITabWidget
  virtual void showWindow();
  virtual void closeWindow();
  virtual QString tabWidgetId() const;
  //IChatWindow
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IMenuBarWidget *menuBarWidget() const { return FMenuBarWidget; }
  virtual IToolBarWidget *toolBarWidget() const { return FToolBarWidget; }
  virtual IStatusBarWidget *statusBarWidget() const { return FStatusBarWidget; }
  virtual bool isActive() const;
  virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle);
signals:
  //ITabWidget
  virtual void windowShow();
  virtual void windowClose();
  virtual void windowChanged();
  virtual void windowDestroyed();
  //IChatWindow
  virtual void messageReady();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void windowActivated();
  virtual void windowClosed();
protected:
  void initialize();
  void saveWindowGeometry();
  void loadWindowGeometry();
protected:
  virtual bool event(QEvent *AEvent);
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
  void onMessageReady();
  void onStreamJidChanged(const Jid &ABefour);
private:
  Ui::ChatWindowClass ui;
private:
  IMessageWidgets *FMessageWidgets;
  IStatusChanger *FStatusChanger;
  ISettings *FSettings;
private:
  IInfoWidget *FInfoWidget;
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
  IMenuBarWidget *FMenuBarWidget;
  IToolBarWidget *FToolBarWidget;
  IStatusBarWidget *FStatusBarWidget;
private:
  Jid FStreamJid;
  Jid FContactJid;
  bool FShownDetached;
};

#endif // CHATWINDOW_H
