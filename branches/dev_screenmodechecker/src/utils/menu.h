#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include "utilsexport.h"
#include "action.h"
#include "iconstorage.h"

#define AG_NULL                   -1
#define AG_DEFAULT                500

class Action;

class UTILS_EXPORT Menu :
	public QMenu
{
	Q_OBJECT;
public:
	Menu(QWidget *AParent = NULL);
	~Menu();
	bool isEmpty() const;
	Action *menuAction() const;
	int actionGroup(const Action *AAction) const;
	QAction *nextGroupSeparator(int AGroup) const;
	QList<Action *> groupActions(int AGroup = AG_NULL) const;
	QList<Action *> findActions(const QMultiHash<int, QVariant> &AData, bool ASearchInSubMenu = false) const;
	void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
	void addMenuActions(const Menu *AMenu, int AGroup = AG_DEFAULT, bool ASort = false);
	void removeAction(Action *AAction);
	void clear();
	void setIcon(const QIcon &AIcon);
	void setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex = 0);
	void setTitle(const QString &ATitle);
signals:
	void actionInserted(QAction *ABefore, Action *AAction, int AGroup, bool ASort);
	void actionRemoved(Action *AAction);
	void separatorInserted(Action *ABefore, QAction *ASeparator);
	void separatorRemoved(QAction *ASeparator);
	void menuDestroyed(Menu *AMenu);
public:
	static bool copyStandardMenu(Menu *ADestination, QMenu *ASource, int AGroup = AG_DEFAULT);
protected slots:
	void onActionDestroyed(Action *AAction);
private:
	Action *FMenuAction;
	IconStorage *FIconStorage;
private:
	QMap<int, QAction *> FSeparators;
	QMultiMap<int, Action *> FActions;
};

#endif // MENU_H
