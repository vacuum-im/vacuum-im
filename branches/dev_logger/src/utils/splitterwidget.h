#ifndef SPLITTERWIDGET_H
#define SPLITTERWIDGET_H

#include <QFrame>
#include <QSplitter>
#include "utilsexport.h"
#include "boxwidget.h"

class UTILS_EXPORT SplitterWidget :
	public QFrame
{
	Q_OBJECT;
public:
	SplitterWidget(QWidget *AParent=NULL, Qt::Orientation AOrientation=Qt::Vertical);
	~SplitterWidget();
	// Widget Management
	QList<QWidget *> widgets() const;
	int widgetHandle(QWidget *AWidget) const;
	int widgetOrder(QWidget *AWidget) const;
	QWidget *widgetByOrder(int AOrderId) const;
	void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0, int AHandle=-1);
	void removeWidget(QWidget *AWidget);
	// Handle Management
	QList<int> handles() const;
	void insertHandle(int AOrderId);
	void removeHandle(int AOrderId);
	int handleWidth() const;
	void setHandleWidth(int AWidth);
	int handleSize(int AOrderId) const;
	void setHandleSize(int AOrderId, int ASize);
	QList<int> handleSizes() const;
	void setHandleSizes(const QList<int> &ASizes);
	bool isHandlesCollaplible() const;
	void setHandlesCollapsible(bool ACollapsible);
	void getHandleRange(int AOrderId, int *AMin, int *AMax) const;
	bool isHandleCollapsible(int AOrderId) const;
	void setHandleCollapsible(int AOrderId, bool ACollapsible);
	bool isHandleStretchable(int AOrderId) const;
	void setHandleStretchable(int AOrderId, bool AStreatchable);
	// Layout Management
	int spacing() const;
	void setSpacing(int ASpace);
	Qt::Orientation orientation() const;
	void setOrientation(Qt::Orientation AOrientation);
signals:
	void handleMoved(int AOrderId, int ASize);
	void widgetInserted(QWidget *AWidget);
	void widgetRemoved(QWidget *AWidget);
protected:
	void updateStretchFactor();
	void saveHandleFixedSizes();
	void adjustBoxWidgetSizes(int ADeltaSize);
protected:
	BoxWidget *getHandleBox(int AOrderId);
	int findWidgetHandle(int AOrderId) const;
	int handleOrderByIndex(int AIndex) const;
	int handleIndexByOrder(int AOrderId) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onSplitterMoved(int APosition, int AIndex);
	void onBoxWidgetInserted(int AOrderId, QWidget *AWidget);
	void onBoxWidgetRemoved(QWidget *AWidget);
private:
	bool FMovingWidgets;
	QSplitter *FSplitter;
	QMap<int, int> FHandleSizes;
	QMap<int, QWidget *> FWidgetOrders;
	QMap<QWidget *, int> FWidgetHandles;
	QMap<int, BoxWidget *> FHandleWidgets;
};

#endif // SPLITTERWIDGET_H
