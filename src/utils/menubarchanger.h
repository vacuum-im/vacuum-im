#ifndef MENUBARCHANGER_H
#define MENUBARCHANGER_H

#include <QMenuBar>
#include <QMultiMap>
#include "utilsexport.h"
#include "menu.h"

#define MBG_NULL        -1
#define MBG_DEFAULT     500

class UTILS_EXPORT MenuBarChanger :
			public QObject
{
	Q_OBJECT;
public:
	MenuBarChanger(QMenuBar *AMenuBar);
	~MenuBarChanger();
	bool isEmpty() const;
	QMenuBar *menuBar() const;
	int menuGroup(Menu *AMenu) const;
	QList<Menu *> groupMenus(int AGroup = MBG_NULL) const;
	void insertMenu(Menu *AMenu, int AGroup = MBG_DEFAULT);
	void removeMenu(Menu *AMenu);
	void clear();
signals:
	void menuInserted(Menu *ABefour, Menu *AMenu, int AGroup);
	void menuRemoved(Menu *AMenu);
	void menuBarChangerDestroyed(MenuBarChanger *AMenuBarChanger);
protected slots:
	void onMenuDestroyed(Menu *AMenu);
private:
	QMenuBar *FMenuBar;
	QMultiMap<int, Menu *> FMenu;
};

#endif // MENUBARCHANGER_H
