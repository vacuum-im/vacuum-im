#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT WidgetManager
{
public:
	static void raiseWidget(QWidget *AWidget);
	static bool isActiveWindow(const QWidget *AWindow);
	static void showActivateRaiseWindow(QWidget *AWindow);
	static void setWindowSticky(QWidget *AWindow, bool ASticky);
	static void alertWidget(QWidget *AWidget);
	static bool isWidgetAlertEnabled();
	static void setWidgetAlertEnabled(bool AEnabled);
	static Qt::Alignment windowAlignment(const QWidget *AWindow);
	static bool alignWindow(QWidget *AWindow, Qt::Alignment AAlign);
	static QRect alignRect(const QRect &ARect, const QRect &ABoundary, Qt::Alignment AAlign=Qt::AlignCenter);
	static QRect alignGeometry(const QSize &ASize, const QWidget *AWidget=NULL, Qt::Alignment AAlign=Qt::AlignCenter);
};

#endif //WIDGETMANAGER_H
