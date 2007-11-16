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
  FIconset = AFileName;
  SkinIconset *iconset = Skin::getSkinIconset(AFileName);
  setIcon(iconset->iconByFile(iconset->iconFiles().value(0)));
  connect(iconset,SIGNAL(iconsetChanged()),SLOT(onSkinIconsetChanged()));
}

QSize SelectIconMenu::sizeHint() const
{
  return FLayout->sizeHint();
}

void SelectIconMenu::createWidget()
{
  destroyWidget();
  FWidget = new SelectIconWidget(FIconset,this);
  FLayout->addWidget(FWidget);
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
  setIcon(iconset->iconByFile(iconset->iconFiles().value(0)));
  setEnabled(iconset->isValid());
}

