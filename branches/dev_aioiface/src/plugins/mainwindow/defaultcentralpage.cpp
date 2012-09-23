#include "defaultcentralpage.h"

DefaultCentralPage::DefaultCentralPage(QWidget *AParent) : QLabel(AParent)
{
	setWordWrap(true);
	setAlignment(Qt::AlignCenter);
	setText(tr("Here will be opened the windows with conversations."));
}

DefaultCentralPage::~DefaultCentralPage()
{
	emit centralPageDestroyed();
}

void DefaultCentralPage::showCentralPage(bool AMinimized)
{
	emit centralPageShow(AMinimized);
}

QIcon DefaultCentralPage::centralPageIcon() const
{
	return QIcon();
}

QString DefaultCentralPage::centralPageCaption() const
{
	return QString::null;
}
