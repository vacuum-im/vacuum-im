#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <QWidget>
#include "utilsexport.h"

class UTILS_EXPORT WidgetManager
{
public:
	static void raiseWidget(QWidget *AWidget);
	static void showActivateRaiseWindow(QWidget *AWindow);
	static QRect alignGeometry(const QSize &ASize, const QWidget *AWidget=NULL, Qt::Alignment AAlign=Qt::AlignCenter);
};

#endif //WIDGETMANAGER_H
