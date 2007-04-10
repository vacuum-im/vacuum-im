#include <QtDebug>
#include "action.h"

int Action::FNewRole = Action::DR_UserDefined + 1;

Action::Action(QObject *AParent)
  : QAction(AParent)
{
  FMenu = NULL;
  FIconset = NULL;
}

Action::~Action()
{
  emit actionDestroyed(this);
}

void Action::setIcon(const QIcon &AIcon)
{
  if (FIconset)
  {
    FIconName.clear();
    delete FIconset;
    FIconset = NULL;
  }
  QAction::setIcon(AIcon);
}

void Action::setIcon(const QString &AIconsetFile, const QString &AIconName)
{
  if (!FIconset)
  {
    FIconset = new SkinIconset(AIconsetFile,this);
    connect(FIconset,SIGNAL(skinChanged()),SLOT(onSkinChanged()));
  }
  FIconName = AIconName;
  FIconset->openFile(AIconsetFile);
  QAction::setIcon(FIconset->iconByName(AIconName));
}

void Action::setMenu(Menu *AMenu)
{
  if (FMenu)
  {
    disconnect(FMenu,SIGNAL(menuDestroyed(Menu *)),this,SLOT(onMenuDestroyed(Menu *)));
    if (FMenu != AMenu && FMenu->parent()==this)
      delete FMenu;
  }

  if (AMenu)
  {
    //setIcon(AMenu->icon());
    //setText(AMenu->title());
    //setToolTip(AMenu->toolTip());
    //setWhatsThis(AMenu->whatsThis());
    connect(AMenu,SIGNAL(menuDestroyed(Menu *)),SLOT(onMenuDestroyed(Menu *)));
  }
  QAction::setMenu(AMenu);
  FMenu = AMenu;
}

void Action::setData(int ARole, const QVariant &AData)
{
  if (AData.isValid())
    FData.insert(ARole,AData);
  else
    FData.remove(ARole);
}

void Action::setData(const QHash<int,QVariant> &AData)
{
  FData.unite(AData);
}

QVariant Action::data(int ARole) const
{
  return FData.value(ARole);
}

int Action::newRole()
{
  FNewRole++;
  return FNewRole;
}

void Action::onMenuDestroyed(Menu *AMenu)
{
  if (AMenu == FMenu)
    FMenu = NULL;
}

void Action::onSkinChanged()
{
  if (FIconset)
    QAction::setIcon(FIconset->iconByName(FIconName));
}
