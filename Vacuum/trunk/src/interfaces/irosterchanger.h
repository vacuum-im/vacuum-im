#ifndef IROSTERCHANGER_H
#define IROSTERCHANGER_H

#include <QDateTime>
#include <QToolBar>
#include <QButtonGroup>
#include <QTextEdit>
#include "../../utils/jid.h"
#include "../../utils/menu.h"
#include "../../utils/toolbarchanger.h"

#define ROSTERCHANGER_UUID "{018E7891-2743-4155-8A70-EAB430573500}"

class ISubscriptionDialog
{
public:
  enum ButtonId {
    NextButton,
    AskButton,
    AuthButton,
    RefuseButton,
    RejectButton,
    CloseButton
  };
public:
  virtual QWidget *instance() =0;
  virtual const Jid &streamJid() const =0;
  virtual const Jid &contactJid() const =0;
  virtual const QDateTime &dateTime() const =0;
  virtual int subsType() const =0;
  virtual const QString &status() const =0;
  virtual const QString &subscription() const =0;
  virtual ToolBarChanger *toolBarChanger() const =0;
  virtual QTextEdit *textEditor() const =0;
  virtual QButtonGroup *buttonGroup() const =0;
signals:
  virtual void dialogChanged() =0;
};

class IRosterChanger
{
public:
  virtual QObject *instance() =0;
  virtual void showAddContactDialog(const Jid &AStreamJid, const Jid &AJid, const QString &ANick,
    const QString &AGroup, const QString &ARequest) =0;
signals:
  virtual void subscriptionDialogCreated(ISubscriptionDialog *ADialog) =0;
  virtual void subscriptionDialogDestroyed(ISubscriptionDialog *ADialog) =0;
};

Q_DECLARE_INTERFACE(ISubscriptionDialog,"Vacuum.Plugin.ISubscriptionDialog/1.0")
Q_DECLARE_INTERFACE(IRosterChanger,"Vacuum.Plugin.IRosterChanger/1.0")

#endif
