#ifndef RECEIVERSWIDGET_H
#define RECEIVERSWIDGET_H

#include <QWidget>
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../utils/jid.h"
#include "ui_receiverswidget.h"

class ReceiversWidget : 
  public IReceiversWidget
{
  Q_OBJECT;
  Q_INTERFACES(IReceiversWidget);
public:
  ReceiversWidget(IMessenger *AMessenger, const Jid &AStreamJid);
  ~ReceiversWidget();
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual QList<Jid> receivers() const { return FReceivers; }
  virtual QString receiverName(const Jid &AReceiver) const;
  virtual void addReceiversGroup(const QString &AGroup);
  virtual void removeReceiversGroup(const QString &AGroup);
  virtual void addReceiver(const Jid &AReceiver);
  virtual void removeReceiver(const Jid &AReceiver);
signals:
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void receiverAdded(const Jid &AReceiver);
  virtual void receiverRemoved(const Jid &AReceiver);
protected:
  void initialize();
  QTreeWidgetItem *getReceiversGroup(const QString &AGroup);
  QTreeWidgetItem *getReceiver(const Jid &AReceiver, const QString &AName, QTreeWidgetItem *AParent);
  void createRosterTree();
protected slots:
  void onReceiversItemChanged(QTreeWidgetItem *AItem, int AColumn);
  void onSelectAllClicked();
  void onSelectNoneClicked();
  void onAddClicked();
  void onUpdateClicked();
private:
  Ui::ReceiversWidgetClass ui;
private:
  IMessenger *FMessenger;
  IRoster *FRoster;
  IPresence *FPresence;
  IStatusIcons *FStatusIcons;
private:
  Jid FStreamJid;
  QList<Jid> FReceivers;
  QHash<QString,QTreeWidgetItem *> FGroupItems;
  QMultiHash<Jid,QTreeWidgetItem *> FContactItems;
};

#endif // RECEIVERSWIDGET_H
