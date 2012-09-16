#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>

class TabWidget : 
	public QTabWidget
{
	Q_OBJECT;
public:
	TabWidget(QWidget *AParent);
	~TabWidget();
protected:
	void tabInserted(int AIndex);
	void tabRemoved(int AIndex);
};

#endif // TABWIDGET_H
