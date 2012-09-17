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
	virtual QList<QWidget *> tabPages() const;
	virtual int tabPageOrder(QWidget *APage) const;
	virtual QWidget *tabPageByOrder(int AOrderId) const;
	virtual QWidget *currentTabPage() const;
	virtual void setCurrentTabPage(QWidget *APage);
	virtual void insertTabPage(int AOrderId, QWidget *APage);
	virtual void updateTabPage(QWidget *APage);
	virtual void removeTabPage(QWidget *APage);
signals:
	void currentTabPageChanged(QWidget *APage);
	void tabPageInserted(int AOrderId, QWidget *APage);
	void tabPageUpdated(QWidget *APage);
	void tabPageRemoved(QWidget *APage);
protected:
	void tabInserted(int AIndex);
	void tabRemoved(int AIndex);
private:
	QMap<int, QWidget *> FTabPageOrders;
};

#endif // MAINTABWIDGET_H
