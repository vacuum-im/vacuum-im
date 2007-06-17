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

  void setupDialog(const Jid &AStreamJid, const Jid &AContactJid, QDateTime ATime, 
    IRoster::SubsType ASubsType, const QString &AStatus, const QString &ASubs);
  void setNextCount(int ANext);
  Action *dialogAction() const { return FDialogAction; }
  
  //ISubscriptionDialog
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual const QDateTime &dateTime() const { return FDateTime; }
  virtual int subsType() const { return FSubsType; }
  virtual const QString &status() const { return FStatus; }
  virtual const QString &subscription() const { return FSubscription; }
  virtual QTextEdit *message() const { return tedMessage; }
  virtual QToolBar *toolBar() const { return FToolBar; }
  virtual QButtonGroup *buttonGroup() const { return FButtonGroup; }
signals:
  virtual void dialogReady();
  virtual void setupNext();
protected slots:
  void onButtonClicked(int AId);
private:
  Action *FDialogAction;
  QToolBar *FToolBar;
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
