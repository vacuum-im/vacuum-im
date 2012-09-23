#ifndef MAINTABWIDGET_H
#define MAINTABWIDGET_H

#include <QTabWidget>

class TabWidget :
			public QTabWidget
{
	Q_OBJECT;
public:
	TabWidget(QWidget *AParent = NULL);
	~TabWidget();
	bool isTabBarVisible() const;
	void setTabBarVisible(bool AVisible);
signals:
	void tabMoved(int AFrom, int ATo);
	void tabMenuRequested(int AIndex);
protected:
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
	void onTabBarContextMenuRequested(const QPoint &APos);
private:
	bool FTabBarVisible;
	int FPressedTabIndex;
};

#endif // TABWIDGET_H
