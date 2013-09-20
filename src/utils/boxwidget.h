#ifndef BOXWIDGET_H
#define BOXWIDGET_H

#include <QMap>
#include <QFrame>
#include <QBoxLayout>
#include "utilsexport.h"

class UTILS_EXPORT BoxWidget : 
	public QFrame
{
	Q_OBJECT;
public:
	BoxWidget(QWidget *AParent=NULL, QBoxLayout::Direction ADirection=QBoxLayout::TopToBottom);
	~BoxWidget();
	// Widget Management
	bool isEmpty() const;
	QList<QWidget *> widgets() const;
	int widgetOrder(QWidget *AWidget) const;
	QWidget *widgetByOrder(int AOrderId) const;
	void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0);
	void removeWidget(QWidget *AWidget);
	// Layout Management
	int spacing() const;
	void setSpacing(int ASpace);
	int stretch(QWidget *AWidget) const;
	void setStretch(QWidget *AWidget, int AStretch);
	QBoxLayout::Direction direction() const;
	void setDirection(QBoxLayout::Direction ADirection);
signals:
	void widgetInserted(int AOrderId, QWidget *AWidget);
	void widgetRemoved(QWidget *AWidget);
protected slots:
	void onWidgetDestroyed(QObject *AObject);
private:
	QBoxLayout *FLayout;
	QMap<int, QWidget *> FWidgetOrders;
};

#endif // BOXWIDGET_H
