#ifndef MAINCENTRALWIDGET_H
#define MAINCENTRALWIDGET_H

#include <QStackedWidget>
#include <interfaces/imainwindow.h>

class MainCentralWidget : 
	public QStackedWidget,
	public IMainCentralWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMainCentralWidget);
public:
	MainCentralWidget(IMainWindow *AMainWindow, QWidget *AParent);
	~MainCentralWidget();
	virtual QStackedWidget *instance() { return this; }
	virtual QList<IMainCentralPage *> centralPages() const;
	virtual IMainCentralPage *currentCentralPage() const;
	virtual void setCurrentCentralPage(IMainCentralPage *APage);
	virtual void appendCentralPage(IMainCentralPage *APage);
	virtual void removeCentralPage(IMainCentralPage *APage);
signals:
	void currentCentralPageChanged(IMainCentralPage *APage);
	void centralPageAppended(IMainCentralPage *APage);
	void centralPageRemoved(IMainCentralPage *APage);
protected slots:
	void onCurrentIndexChanged(int AIIndex);
	void onCentralPageShow(bool AMinimized);
	void onCentralPageChanged();
	void onCentralPageDestroyed();
private:
	IMainWindow *FMainWindow;
private:
	QList<IMainCentralPage *> FCentralPages;
};

#endif // MAINCENTRALWIDGET_H
