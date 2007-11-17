#include <QtDebug>
#include "selecticonmenu.h"

#define SMILEY_TAG_NAME                         "text"

SelectIconMenu::SelectIconMenu(QWidget *AParent)
  : Menu(AParent)
{
  FWidget = NULL;

  FLayout = new QVBoxLayout(this);
  FLayout->setMargin(0);
  connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
}

SelectIconMenu::~SelectIconMenu()
{

}

Action *SelectIconMenu::menuAction()
{
  return Menu::menuAction();
}

void SelectIconMenu::setIcon(const QIcon &AIcon)
{
  Menu::setIcon(AIcon);
}

void SelectIconMenu::setTitle(const QString &ATitle)
{
  Menu::setTitle(ATitle);
}

void SelectIconMenu::setIconset(const QString &AFileName)
{
  if (!FIconset.isEmpty())
  {
    SkinIconset *iconset = Skin::getSkinIconset(FIconset);
    disconnect(iconset,SIGNAL(iconsetChanged()),this,SLOT(onSkinIconsetChanged()));
  }
  FIconset = AFileName;
  if (!FIconset.isEmpty())
  {
    SkinIconset *iconset = Skin::getSkinIconset(FIconset);
    connect(iconset,SIGNAL(iconsetChanged()),SLOT(onSkinIconsetChanged()));
    onSkinIconsetChanged();
  }
  else
    setEnabled(false);
}

QSize SelectIconMenu::sizeHint() const
{
  return FSizeHint;
}

void SelectIconMenu::createWidget()
{
  destroyWidget();
  FWidget = new SelectIconWidget(FIconset,this);
  FLayout->addWidget(FWidget);
  FSizeHint = FLayout->sizeHint();
  connect(FWidget,SIGNAL(iconSelected(const QString &, const QString &)),SIGNAL(iconSelected(const QString &, const QString &)));
}

void SelectIconMenu::destroyWidget()
{
  delete FWidget;
  FWidget = NULL;
}

void SelectIconMenu::hideEvent(QHideEvent *AEvent)
{
  destroyWidget();
  Menu::hideEvent(AEvent);
}

void SelectIconMenu::onAboutToShow()
{
  createWidget();
}

void SelectIconMenu::onSkinIconsetChanged()
{
  SkinIconset *iconset = Skin::getSkinIconset(FIconset);
  setTitle(iconset->tags().value(0));
  setIcon(iconset->iconByFile(iconset->iconFiles().value(0)));
  setEnabled(iconset->isValid());
}

