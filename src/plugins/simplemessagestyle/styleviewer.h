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
protected:
	void mousePressEvent(QMouseEvent *AEvent) override;
public slots:
	void onLinkActivated(const QUrl &AUrl);
};

#endif // STYLEVIEWER_H
