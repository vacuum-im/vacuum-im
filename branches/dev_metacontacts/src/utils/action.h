#ifndef ACTION_H
#define ACTION_H

#include <QHash>
#include <QAction>
#include <QVariant>
#include "utilsexport.h"
#include "menu.h"
#include "shortcuts.h"
#include "iconstorage.h"

class Menu;

class UTILS_EXPORT Action :
			public QAction
{
	Q_OBJECT;
public:
	enum DataRoles {
		DR_Parametr1,
		DR_Parametr2,
		DR_Parametr3,
		DR_Parametr4,
		DR_StreamJid,
		DR_SortString,
		DR_UserDefined = 64,
	};
public:
	Action(QObject *AParent = NULL);
	~Action();
	//QAction
	Menu *menu() const;
	void setMenu(Menu *AMenu);
	void setIcon(const QIcon &AIcon);
	void setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex = 0);
	//Action
	QVariant data(int ARole) const;
	void setData(int ARole, const QVariant &AData);
	void setData(const QHash<int, QVariant> &AData);
	QString shortcutId() const;
	void setShortcutId(const QString &AId);
signals:
	void actionDestroyed(Action *AAction);
public:
	static QString reduceString(const QString &AString, int AMaxChars);
	static bool copyStandardAction(Action *ADestination, QAction *ASource);
protected slots:
	void onMenuDestroyed(Menu *AMenu);
private:
	Menu *FMenu;
	IconStorage *FIconStorage;
private:
	QString FShortcutId;
	QHash<int, QVariant> FData;
};

#endif // ACTION_H
