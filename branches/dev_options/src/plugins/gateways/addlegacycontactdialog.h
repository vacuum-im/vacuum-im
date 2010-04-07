#ifndef ADDLEGACYCONTACTDIALOG_H
#define ADDLEGACYCONTACTDIALOG_H

#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/igateways.h>
#include <interfaces/irosterchanger.h>
#include <utils/iconstorage.h>
#include "ui_addlegacycontactdialog.h"

class AddLegacyContactDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  AddLegacyContactDialog(IGateways *AGateways, IRosterChanger *ARosterChanger, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
  ~AddLegacyContactDialog();
public:
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &serviceJid() const { return FServiceJid; }
protected:
  void resetDialog();
  void requestPrompt();
  void requestUserJid();
protected slots:
  void onPromptReceived(const QString &AId, const QString &ADesc, const QString &APrompt);
  void onUserJidReceived(const QString &AId, const Jid &AUserJid);
  void onErrorReceived(const QString &AId, const QString &AError);
  void onDialogButtonsClicked(QAbstractButton *AButton);
private:
  Ui::AddLegacyContactDialogClass ui;
private:
  IGateways *FGateways;
  IRosterChanger *FRosterChanger;
private:
  Jid FStreamJid;
  Jid FServiceJid;
  QString FContactId;
  QString FRequestId;
};

#endif // ADDLEGACYCONTACTDIALOG_H
