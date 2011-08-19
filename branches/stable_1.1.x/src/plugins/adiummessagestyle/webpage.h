#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebPage>

class WebPage :
			public QWebPage
{
	Q_OBJECT;
public:
	WebPage(QObject *AParent = NULL);
	~WebPage();
	// QWebPage
	virtual void triggerAction(WebAction AAction, bool AChecked = false);
};

#endif // WEBPAGE_H
