#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/shortcuts.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>
#include "ui_tabwindow.h"

class TabWindow :
			public QMainWindow,
			public ITabWindow
{
	Q_OBJECT;
	Q_INTERFACES(ITabWindow);
public:
	TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId);
	~TabWindow();
	virtual QMainWindow *instance() { return this; }
	virtual void showWindow();
	virtual QUuid windowId() const;
	virtual QString windowName() const;
	virtual Menu *windowMenu() const;
	virtual void addPage(ITabWindowPage *APage);
	virtual bool hasPage(ITabWindowPage *APage) const;
	virtual ITabWindowPage *currentPage() const;
	virtual void setCurrentPage(ITabWindowPage *APage);
	virtual void detachPage(ITabWindowPage *APage);
	virtual void removePage(ITabWindowPage *APage);
	virtual void clear();
signals:
	void pageAdded(ITabWindowPage *APage);
	void currentPageChanged(ITabWindowPage *APage);
	void pageRemoved(ITabWindowPage *APage);
	void pageDetached(ITabWindowPage *APage);
	void windowChanged();
	void windowDestroyed();
protected:
	void createActions();
	void saveWindowStateAndGeometry();
	void loadWindowStateAndGeometry();
	void updateWindow();
	void updateTab(int AIndex);
	void updateTabs(int AFrom, int ATo);
protected slots:
	void onTabMoved(int AFrom, int ATo);
	void onTabChanged(int AIndex);
	void onTabCloseRequested(int AIndex);
	void onTabPageShow();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabWindowAppended(const QUuid &AWindowId, const QString &AName);
	void onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
	void onTabWindowDeleted(const QUuid &AWindowId);
	void onOptionsChanged(const OptionsNode &ANode);
	void onActionTriggered(bool);
private:
	Ui::TabWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FWindowMenu;
	Menu *FJoinMenu;
	Action *FCloseTab;
	Action *FNextTab;
	Action *FPrevTab;
	Action *FNewTab;
	Action *FDetachWindow;
	Action *FShowCloseButtons;
	Action *FTabsBottom;
	Action *FShowIndices;
	Action *FRemoveTabsOnClose;
	Action *FSetAsDefault;
	Action *FRenameWindow;
	Action *FCloseWindow;
	Action *FDeleteWindow;
private:
	bool FShowTabIndices;
private:
	QUuid FWindowId;
	OptionsNode FOptionsNode;
};

#endif // TABWINDOW_H
