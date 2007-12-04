#ifndef CLIENTINFODIALOG_H
#define CLIENTINFODIALOG_H

#include <QDialog>
#include "../../interfaces/iclientinfo.h"
#include "ui_clientinfodialog.h"

class ClientInfoDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  ClientInfoDialog(const QString &AContactName, const Jid &AContactJid, const Jid &AStreamJid, IClientInfo *AClientInfo, QWidget *AParent = NULL);
  ~ClientInfoDialog();
  const Jid &streamJid() const { return FStreamJid; }
  const Jid &contactJid() const { return FContactJid; }
signals:
  void clientInfoDialogClosed(const Jid &FContactJid);
protected:
  void updateText();
protected slots:
  void onClientInfoChanged(const Jid &AConatctJid);
private:
  Ui::ClientInfoDialogClass ui;
private:
  QString FContactName;
  Jid FContactJid;
  Jid FStreamJid;
  IClientInfo *FClientInfo;
};

#endif // CLIENTINFODIALOG_H
