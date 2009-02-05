#include "selecticonmenu.h"

SelectIconMenu::SelectIconMenu(QWidget *AParent) : Menu(AParent)
{
  FWidget = NULL;
  FStorage = NULL;
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

QString SelectIconMenu::iconset() const
{
  return FStorage!=NULL ? FStorage->subStorage() : "";
}

void SelectIconMenu::setIconset(const QString &ASubStorage)
{
  if (FStorage==NULL || FStorage->subStorage()!=ASubStorage)
  {
    delete FStorage;
    FStorage = new IconStorage(RSR_STORAGE_EMOTICONS,ASubStorage,this);
    QString firstKey = FStorage->fileKeys().value(0);
    FStorage->insertAutoIcon(this,firstKey);
  }
}

QSize SelectIconMenu::sizeHint() const
{
  return FSizeHint;
}

void SelectIconMenu::createWidget()
{
  destroyWidget();
  FWidget = new SelectIconWidget(FStorage,this);
  FLayout->addWidget(FWidget);
  FSizeHint = FLayout->sizeHint();
  connect(FWidget,SIGNAL(iconSelected(const QString &, const QString &)),SIGNAL(iconSelected(const QString &, const QString &)));
}

void SelectIconMenu::destroyWidget()
{
  if (FWidget)
  {
    FWidget->deleteLater();
    FWidget = NULL;
  }
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

