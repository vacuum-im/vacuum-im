#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>

class TabWidget :
			public QTabWidget
{
	Q_OBJECT;
public:
	TabWidget(QWidget *AParent = NULL);
	~TabWidget();
signals:
	void tabMoved(int AFrom, int ATo);
	void tabMenuRequested(int AIndex);
protected:
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
	void onTabBarContextMenuRequested(const QPoint &APos);
private:
	int FPressedTabIndex;
};

#endif // TABWIDGET_H
