#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QDialog>
#include "../../definations/actiongroups.h"
#include "../../definations/vcardvaluenames.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ivcard.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../utils/action.h"
#include "ui_addcontactdialog.h"

class AddContactDialog : 
  public QDialog, 
  public IAddContactDialog
{
  Q_OBJECT;
  Q_INTERFACES(IAddContactDialog);
public:
  AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
  ~AddContactDialog();
  //IAddContactDialog
  virtual QDialog *instance() { return this; }
  virtual Jid streamJid() const;
  virtual Jid contactJid() const;
  virtual void setContactJid(const Jid &AContactJid);
  virtual QString nickName() const;
  virtual void setNickName(const QString &ANick);
  virtual QString group() const;
  virtual void setGroup(const QString &AGroup);
  virtual bool subscribeContact() const;
  virtual void setSubscribeContact(bool ASubscribe);
  virtual QString subscriptionMessage() const;
  virtual void setSubscriptionMessage(const QString &AMessage);
  virtual ToolBarChanger *toolBarChanger() const;
signals:
  virtual void dialogDestroyed();
protected:
  void initialize(IPluginManager *APluginManager);
protected slots:
  void onDialogAccepted();
  void onToolBarActionTriggered(bool);
  void onVCardReceived(const Jid &AContactJid);
private:
  Ui::AddContactDialogClass ui;
private:
  IRoster *FRoster;
  IMessenger *FMessenger;
  IVCardPlugin *FVcardPlugin;
  IRosterChanger *FRosterChanger;
private:
  Action *FShowChat;
  Action *FSendMessage;
  Action *FShowVCard;
  Action *FResolve;
private:
  bool FResolving;
  Jid FStreamJid;
  ToolBarChanger *FToolBarChanger;
};

#endif // ADDCONTACTDIALOG_H
