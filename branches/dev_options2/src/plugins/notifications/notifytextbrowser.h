#ifndef NOTIFYTEXTBROWSER_H
#define NOTIFYTEXTBROWSER_H

#include <QTextBrowser>
#include <utils/animatedtextbrowser.h>

class NotifyTextBrowser : 
	public AnimatedTextBrowser
{
	Q_OBJECT;
public:
	NotifyTextBrowser(QWidget *AParent);
	~NotifyTextBrowser();
	void setMaxHeight(int AMax);
public:
	void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
	void onTextChanged();
private:
	int FMaxHeight;
};

#endif // NOTIFYTEXTBROWSER_H
