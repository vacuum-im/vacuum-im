#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QTextBrowser>
#include <utils/animatedtextbrowser.h>

class StyleViewer: 
	public AnimatedTextBrowser
{
	Q_OBJECT;
public:
	StyleViewer(QWidget *AParent);
	~StyleViewer();
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
};

#endif // STYLEVIEWER_H
