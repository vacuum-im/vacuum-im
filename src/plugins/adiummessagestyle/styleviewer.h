#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QWebView>
#include "webpage.h"

class StyleViewer :
	public QWebView
{
	Q_OBJECT;
public:
	StyleViewer(QWidget *AParent);
	~StyleViewer();
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
protected slots:
	void onShortcutActivated();
};

#endif // STYLEVIEWER_H
