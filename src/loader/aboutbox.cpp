#include "aboutbox.h"

#include <QDesktopServices>
#include <utils/logger.h>

AboutBox::AboutBox(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.lblName->setText(CLIENT_NAME);

	if (APluginManager->revisionDate().isValid())
	{
		QString revDate = APluginManager->revisionDate().date().toString(Qt::SystemLocaleShortDate);
		ui.lblVersion->setText(tr("Version: %1 %2 of %3").arg(APluginManager->version(),CLIENT_VERSION_SUFFIX,revDate));
		ui.lblRevision->setText(tr("Revision: %1").arg(APluginManager->revision()));
	}
	else
	{
		ui.lblVersion->setText(tr("Version: %1 %2").arg(APluginManager->version(),CLIENT_VERSION_SUFFIX));
		ui.lblRevision->setVisible(false);
	}

	connect(ui.lblHomePage,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblSourcePage,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
}

AboutBox::~AboutBox()
{

}

void AboutBox::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}
