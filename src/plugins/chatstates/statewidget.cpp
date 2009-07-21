#include "statewidget.h"

#include <QDateTime>

#define ADR_PERMIT_STATUS     Action::DR_Parametr1

StateWidget::StateWidget(IChatStates *AChatStates, IChatWindow *AWindow) : QPushButton(AWindow->statusBarWidget()->statusBarChanger()->statusBar())
{
  FWindow = AWindow;
  FChatStates = AChatStates;

  FMenu = new Menu(this);

  Action *action = new Action(FMenu);
  action->setCheckable(true);
  action->setText(tr("Default"));
  action->setData(ADR_PERMIT_STATUS, IChatStates::StatusDefault);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
  FMenu->addAction(action);

  action = new Action(FMenu);
  action->setCheckable(true);
  action->setText(tr("Always send"));
  action->setData(ADR_PERMIT_STATUS, IChatStates::StatusEnable);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
  FMenu->addAction(action);

  action = new Action(FMenu);
  action->setCheckable(true);
  action->setText(tr("Never send"));
  action->setData(ADR_PERMIT_STATUS, IChatStates::StatusDisable);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusActionTriggered(bool)));
  FMenu->addAction(action);

  setMenu(FMenu);
  setFlat(true);
  setToolTip(tr("User chat status"));

  connect(FChatStates->instance(),SIGNAL(permitStatusChanged(const Jid &, int)),
    SLOT(onPermitStatusChanged(const Jid &, int)));
  connect(FChatStates->instance(),SIGNAL(userChatStateChanged(const Jid &, const Jid &, int)),
    SLOT(onUserChatStateChanged(const Jid &, const Jid &, int)));

  onPermitStatusChanged(FWindow->contactJid(),FChatStates->permitStatus(FWindow->contactJid()));
  onUserChatStateChanged(FWindow->streamJid(),FWindow->contactJid(),FChatStates->userChatState(FWindow->streamJid(),FWindow->contactJid()));
}

StateWidget::~StateWidget()
{

}

QSize StateWidget::sizeHint() const
{
  QSize hint = QPushButton::sizeHint();
  hint.setHeight(qMax(fontMetrics().size(Qt::TextShowMnemonic,text()).height()+2,iconSize().height()));
  return hint;
}

QSize StateWidget::minimumSizeHint() const
{
  return sizeHint();
}

void StateWidget::onStatusActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    FChatStates->setPermitStatus(FWindow->contactJid(),action->data(ADR_PERMIT_STATUS).toInt());
    action->setChecked(true);
  }
}

void StateWidget::onPermitStatusChanged(const Jid &AContactJid, int AStatus)
{
  if (FWindow->contactJid() && AContactJid)
  {
    foreach(Action *action, FMenu->groupActions(AG_DEFAULT))
      action->setChecked(action->data(ADR_PERMIT_STATUS).toInt()==AStatus);
  }
}

void StateWidget::onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState)
{
  if (FWindow->streamJid()==AStreamJid && FWindow->contactJid()==AContactJid)
  {
    QString state = tr("Unknown");
    if (AState == IChatStates::StateActive)
      state = tr("Active");
    else if (AState == IChatStates::StateComposing)
      state = tr("Composing");
    else if (AState == IChatStates::StatePaused)
      state = tr("Paused");
    else if (AState == IChatStates::StateInactive)
      state = tr("Inactive %1").arg(QDateTime::currentDateTime().toString("hh:mm"));
    else if (AState == IChatStates::StateGone)
      state = tr("Gone %1").arg(QDateTime::currentDateTime().toString("hh:mm"));
    setText(state);
    setMinimumWidth(qMax(minimumWidth(),sizeHint().width()));
  }
}
