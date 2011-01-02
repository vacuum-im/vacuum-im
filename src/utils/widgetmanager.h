#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT WidgetManager
{
public:
	static void raiseWidget(QWidget *AWidget);
	static void showActivateRaiseWindow(QWidget *AWindow);
	static void setWindowSticky(QWidget *AWindow, bool ASticky);
	static Qt::Alignment windowAlignment(const QWidget *AWindow);
	static void alignWindow(QWidget *AWindow, Qt::Alignment AAlign);
	static QRect alignGeometry(const QSize &ASize, const QWidget *AWidget=NULL, Qt::Alignment AAlign=Qt::AlignCenter);
};

#endif //WIDGETMANAGER_H
