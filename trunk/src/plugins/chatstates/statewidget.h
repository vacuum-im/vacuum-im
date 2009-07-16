#ifndef SATEWIDGET_H
#define SATEWIDGET_H

#include <QLabel>
#include <QPushButton>
#include "../../interfaces/ichatstates.h"
#include "../../interfaces/imessagewidgets.h"
#include "../../utils/menu.h"

class StateWidget : 
  public QPushButton
{
  Q_OBJECT;
public:
  StateWidget(IChatStates *AChatStates, IChatWindow *AWindow);
  ~StateWidget();
protected slots:
  void onStatusActionTriggered(bool);
  void onPermitStatusChanged(const Jid &AContactJid, int AStatus);
  void onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState);
private:
  IChatWindow *FWindow;
  IChatStates *FChatStates;
private:
  Menu *FMenu;
};

#endif // SATEWIDGET_H
