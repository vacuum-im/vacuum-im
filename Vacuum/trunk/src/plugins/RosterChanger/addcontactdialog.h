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
  AddContactDialog(QWidget *AParent = NULL);
  ~AddContactDialog();
  
  const Jid &stramJid() const;
  Jid contactJid() const;
  QString nick() const;
  QSet<QString> groups() const;
  QString requestText() const;
  bool requestSubscr() const;
  bool sendSubscr() const;

  void setStreamJid(const Jid &AStreamJid);
  void setContactJid(const Jid &AJid);
  void setNick(const QString &ANick);
  void setGroup(const QString &AGroup);
  void setGroups(const QSet<QString> &AGroups);
  void setRequestText(const QString &AText);
signals:
  void addContact(AddContactDialog *);
private slots:
  void onAccepted();
private:
  Jid FStreamJid;
};

#endif // ADDCONTACTDIALOG_H
