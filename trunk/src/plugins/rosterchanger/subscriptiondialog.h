#ifndef SUBSCRIPTIONDIALOG_H
#define SUBSCRIPTIONDIALOG_H

#include <definations/toolbargroups.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/iroster.h>
#include <interfaces/ivcard.h>
#include "ui_subscriptiondialog.h"

class SubscriptionDialog : 
  public QDialog, 
  public ISubscriptionDialog
{
  Q_OBJECT;
  Q_INTERFACES(ISubscriptionDialog);
public:
  SubscriptionDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, 
    const Jid &AContactJid, const QString &ANotify, const QString &AMessage, QWidget *AParent = NULL);
  ~SubscriptionDialog();
  //ISubscriptionDialog
  virtual QDialog *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual QVBoxLayout *actionsLayout() const { return ui.lytActionsLayout; }
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
signals:
  virtual void dialogDestroyed();
protected:
  void initialize(IPluginManager *APluginManager);
protected slots:
  void onDialogAccepted();
  void onDialogRejected();
  void onToolBarActionTriggered(bool);
private:
  Ui::SubscriptionDialogClass ui;
private:
  IRoster *FRoster;
  IMessageProcessor *FMessageProcessor;
  IVCardPlugin *FVcardPlugin;
  IRosterChanger *FRosterChanger;
private:
  Action *FShowChat;
  Action *FSendMessage;
  Action *FShowVCard;
private:
  Jid FStreamJid;
  Jid FContactJid;
  ToolBarChanger *FToolBarChanger;
};

#endif // SUBSCRIPTIONDIALOG_H
