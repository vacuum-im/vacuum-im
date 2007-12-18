#ifndef SUBSCRIPTIONDIALOG_H
#define SUBSCRIPTIONDIALOG_H

#include <QDialog>
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/iroster.h"
#include "../../utils/action.h"
#include "ui_subscriptiondialog.h"

using namespace Ui;

class SubscriptionDialog : 
  public QDialog, 
  public SubscriptionDialogClass,
  public ISubscriptionDialog
{
  Q_OBJECT;
  Q_INTERFACES(ISubscriptionDialog);
public:
  SubscriptionDialog(QWidget *AParent = NULL);
  ~SubscriptionDialog();
  //ISubscriptionDialog
  virtual QWidget *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual const QDateTime &dateTime() const { return FDateTime; }
  virtual int subsType() const { return FSubsType; }
  virtual const QString &status() const { return FStatus; }
  virtual const QString &subscription() const { return FSubscription; }
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
  virtual QTextEdit *textEditor() const { return tedMessage; }
  virtual QButtonGroup *buttonGroup() const { return FButtonGroup; }
signals:
  virtual void dialogChanged();
public:
  void setupDialog(const Jid &AStreamJid, const Jid &AContactJid, QDateTime ATime, 
    IRoster::SubsType ASubsType, const QString &AStatus, const QString &ASubs);
  void setNextCount(int ANext);
  Action *dialogAction() const { return FDialogAction; }
signals:
  void showNext();
protected slots:
  void onButtonClicked(int AId);
private:
  Action *FDialogAction;
  QToolBar *FToolBar;
  ToolBarChanger *FToolBarChanger;
  QButtonGroup *FButtonGroup;
private:
  Jid FStreamJid;
  Jid FContactJid;
  QDateTime FDateTime;
  IRoster::SubsType FSubsType;
  QString FStatus;
  int FNextCount;
  QString FSubscription;
};

#endif // SUBSCRIPTIONDIALOG_H
