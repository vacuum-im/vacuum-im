#ifndef MAINTABWIDGET_H
#define MAINTABWIDGET_H

#include <QTabWidget>
#include <interfaces/imainwindow.h>

class MainTabWidget : 
	public QTabWidget,
	public IMainTabWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMainTabWidget);
public:
	MainTabWidget(QWidget *AParent);
	~MainTabWidget();
	virtual QTabWidget *instance() { return this; }
	virtual QList<IMainTabPage *> tabPages() const;
	virtual int tabPageOrder(IMainTabPage *APage) const;
	virtual IMainTabPage *tabPageByOrder(int AOrderId) const;
	virtual IMainTabPage *currentTabPage() const;
	virtual void setCurrentTabPage(IMainTabPage *APage);
	virtual void insertTabPage(int AOrderId, IMainTabPage *APage);
	virtual void removeTabPage(IMainTabPage *APage);
signals:
	void currentTabPageChanged(IMainTabPage *APage);
	void tabPageInserted(int AOrderId, IMainTabPage *APage);
	void tabPageRemoved(IMainTabPage *APage);
protected:
	void tabInserted(int AIndex);
	void tabRemoved(int AIndex);
protected slots:
	void onTabPageChanged();
	void onTabPageDestroyed();
private:
	QMap<int, IMainTabPage *> FTabPageOrders;
};

#endif // MAINTABWIDGET_H
