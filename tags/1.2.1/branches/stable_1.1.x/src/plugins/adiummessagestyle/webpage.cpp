#include "webpage.h"

#include <QAction>

static const QList<QWebPage::WebAction> AllowedAction = QList<QWebPage::WebAction>() << QWebPage::CopyLinkToClipboard << QWebPage::CopyImageToClipboard << QWebPage::Copy;

WebPage::WebPage(QObject *AParent) : QWebPage(AParent)
{
	setContentEditable(false);
	setNetworkAccessManager(NULL);
	setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

	for (int actionIndex = 0; action((QWebPage::WebAction)actionIndex); actionIndex++)
	{
		if (!AllowedAction.contains((QWebPage::WebAction)actionIndex))
			action((QWebPage::WebAction)actionIndex)->setVisible(false);
	}
}

WebPage::~WebPage()
{

}

void WebPage::triggerAction(WebAction AAction, bool AChecked)
{
	if (AllowedAction.contains(AAction))
		QWebPage::triggerAction(AAction, AChecked);
}
