#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QDialog>
#include <QSet>
#include "ui_addcontactdialog.h"
#include "../../utils/jid.h"

using namespace Ui;

class AddContactDialog : 
  public QDialog, 
  public AddContactDialogClass
{
  Q_OBJECT;
public:
  AddContactDialog(const Jid &AStreamJid, const QSet<QString> &AGroups, QWidget *AParent = NULL);
  ~AddContactDialog();
  //IAddContactDialog  
  const Jid &streamJid() const;
  Jid contactJid() const;
  void setContactJid(const Jid &AJid);
  QString nick() const;
  void setNick(const QString &ANick);
  QSet<QString> groups() const;
  void setGroup(const QString &AGroup);
  QString requestText() const;
  void setRequestText(const QString &AText);
  bool requestSubscription() const;
  bool sendSubscription() const;
signals:
  void addContact(AddContactDialog *ADialog);
private slots:
  void onAccepted();
private:
  Jid FStreamJid;
};

#endif // ADDCONTACTDIALOG_H
