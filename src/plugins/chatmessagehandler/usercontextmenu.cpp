#include "usercontextmenu.h"

UserContextMenu::UserContextMenu(IRostersModel *AModel, IRostersView *AView, IChatWindow *AWindow) : Menu(AWindow->menuBarWidget()->menuBarChanger()->menuBar())
{
  FRosterIndex = NULL;
  FRostersModel = AModel;
  FRostersView = AView;
  FChatWindow = AWindow;

  connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
  connect(this,SIGNAL(aboutToHide()),SLOT(onAboutToHide()));
  connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
  connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *,int)),SLOT(onRosterIndexDataChanged(IRosterIndex *,int)));
  connect(FRostersModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRosterIndexRemoved(IRosterIndex *)));
  connect(FChatWindow->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onChatWindowContactJidChanged(const Jid &)));
  connect(FChatWindow->instance(),SIGNAL(windowChanged()),SLOT(onChatWindowChanged()));

  onRosterIndexRemoved(FRosterIndex);
}

UserContextMenu::~UserContextMenu()
{

}

bool UserContextMenu::isAcceptedIndex(IRosterIndex *AIndex)
{
  if (AIndex!=NULL && FChatWindow->streamJid()==AIndex->data(RDR_STREAM_JID).toString())
  {
    Jid indexJid = AIndex->data(RDR_PJID).toString();
    if (FChatWindow->contactJid() == indexJid)
      return true;
    if (indexJid.resource().isEmpty() && (FChatWindow->contactJid() && indexJid))
      return true;
  }
  return false;
}

void UserContextMenu::updateMenu()
{
  if (FRosterIndex)
  {
    QString name = FRosterIndex->data(RDR_NAME).toString();
    Jid jid = FRosterIndex->data(RDR_PJID).toString();
    if (!jid.resource().isEmpty())
      name += "/" + jid.resource();
    setTitle(name);
    menuAction()->setVisible(true);
  }
  else
  {
    setTitle(FChatWindow->contactJid().full());
    menuAction()->setVisible(false);
  }
  onChatWindowChanged();
}

void UserContextMenu::onAboutToShow()
{
  if (FRosterIndex)
    FRostersView->contextMenuForIndex(this,FRosterIndex,RLID_DISPLAY);
}

void UserContextMenu::onAboutToHide()
{
  clear();
}

void UserContextMenu::onRosterIndexInserted(IRosterIndex *AIndex)
{
  if (FRosterIndex==NULL && isAcceptedIndex(AIndex))
  {
    FRosterIndex = AIndex;
    updateMenu();
  }
}

void UserContextMenu::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
  if (AIndex == FRosterIndex)
  {
    if (ARole == RDR_PJID)
    {
      if (isAcceptedIndex(AIndex))
        updateMenu();
      else
        onRosterIndexRemoved(FRosterIndex);
    }
    else if (ARole == RDR_NAME)
    {
      updateMenu();
    }
  }
  else if (FRosterIndex==NULL && ARole==RDR_PJID && isAcceptedIndex(AIndex))
  {
    FRosterIndex = AIndex;
    updateMenu();
  }
}

void UserContextMenu::onRosterIndexRemoved(IRosterIndex *AIndex)
{
  if (FRosterIndex == AIndex)
  {
    FRosterIndex = FRostersModel->getContactIndexList(FChatWindow->streamJid(),FChatWindow->contactJid()).value(0);
    updateMenu();
  }
}

void UserContextMenu::onChatWindowContactJidChanged(const Jid &/*ABefour*/)
{
  onRosterIndexRemoved(FRosterIndex);
}

void UserContextMenu::onChatWindowChanged()
{
  //setIcon(FChatWindow->instance()->windowIcon());
}
