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
	void tabMoved(int from, int to);
protected:
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
private:
	int FPressedTabIndex;
};

#endif // TABWIDGET_H
