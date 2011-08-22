#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QTextBrowser>

class StyleViewer :
			public QTextBrowser
{
	Q_OBJECT;
public:
	StyleViewer(QWidget *AParent);
	~StyleViewer();
protected:
   void mouseReleaseEvent(QMouseEvent *AEvent);
};

#endif // STYLEVIEWER_H
