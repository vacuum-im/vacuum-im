#include "aboutbox.h"

#include <QDesktopServices>

AboutBox::AboutBox(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.lblName->setText(CLIENT_NAME);
	ui.lblVersion->setText(tr("Version: %1.%2 %3").arg(APluginManager->version()).arg(APluginManager->revision()).arg(CLIENT_VERSION_SUFIX));

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
