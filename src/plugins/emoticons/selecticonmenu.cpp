#include "selecticonmenu.h"

#include <definitions/resources.h>
#include <utils/iconstorage.h>

SelectIconMenu::SelectIconMenu(const QString &AIconset, QWidget *AParent) : Menu(AParent)
{
	FStorage = NULL;
	setIconset(AIconset);

	FScrollArea = new QScrollArea(this);
	FScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	FScrollArea->setWidgetResizable(true);
	setAttribute(Qt::WA_AlwaysShowToolTips,true);
	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
}

SelectIconMenu::~SelectIconMenu()
{

}

QString SelectIconMenu::iconset() const
{
	return FStorage!=NULL ? FStorage->subStorage() : QString();
}

void SelectIconMenu::setIconset(const QString &ASubStorage)
{
	if (FStorage==NULL || FStorage->subStorage()!=ASubStorage)
	{
		delete FStorage;
		FStorage = new IconStorage(RSR_STORAGE_EMOTICONS,ASubStorage,this);
		FStorage->insertAutoIcon(this,FStorage->fileKeys().value(0));
	}
}

QSize SelectIconMenu::sizeHint() const
{
	return FScrollArea->sizeHint();
}

void SelectIconMenu::onAboutToShow()
{
	QWidget *widget = new SelectIconWidget(FStorage,FScrollArea);
	FScrollArea->setWidget(widget);

	connect(this,SIGNAL(aboutToHide()),widget,SLOT(deleteLater()));
	connect(widget,SIGNAL(iconSelected(const QString &, const QString &)),SIGNAL(iconSelected(const QString &, const QString &)));
}
