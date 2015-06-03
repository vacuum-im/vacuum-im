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
	// QMenu
	void clear();
	Action *menuAction() const;
	QList<Action *> actions() const;
	void setIcon(const QIcon &AIcon);
	void setTitle(const QString &ATitle);
	// Menu
	QMenu *carbonMenu() const;
	QList<Action *> actions(int AGroup) const;
	int actionGroup(const Action *AAction) const;
	QAction *nextGroupSeparator(int AGroup) const;
	QList<Action *> findActions(const QMultiHash<int, QVariant> &AData, bool ADeep = false) const;
	void addAction(Action *ABefore, Action *AAction, int AGroup = AG_DEFAULT);
	void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
	void addMenuActions(const Menu *AMenu, int AGroup = AG_NULL, bool ASort = false);
	void removeAction(Action *AAction);
	void setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex = 0);
signals:
	void actionInserted(QAction *ABefore, Action *AAction, int AGroup);
	void actionRemoved(Action *AAction);
	void separatorInserted(Action *ABefore, QAction *ASeparator);
	void separatorRemoved(QAction *ASeparator);
	void menuDestroyed(Menu *AMenu);
public:
	static void copyMenuProperties(Menu *ADestination, QMenu *ASource, int AFirstGroup = AG_DEFAULT);
	static Action *findDuplicateAction(Menu *ADuplicate, QAction *ASource);
	static Menu *duplicateMenu(QMenu *ASource, QWidget *AParent = NULL);
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onCarbonMenuActionChanged();
	void onActionDestroyed(Action *AAction);
private:
	QMenu *FCarbon;
	Action *FMenuAction;
	IconStorage *FIconStorage;
private:
	QMap<int, QAction *> FSeparators;
	QMap<int, QList<Action *> > FActions;
};

#endif // MENU_H
