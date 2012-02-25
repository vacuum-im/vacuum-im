#include "archiveviewwindow.h"

#include <utils/iconstorage.h>

ArchiveViewWindow::ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, IRoster *ARoster, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("View History - %1").arg(FRoster->streamJid().bare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_HISTORY_VIEW,0,0,"windowIcon");

	FRoster = ARoster;
	FArchiver = AArchiver;

	FStatusIcons = NULL;
	FMessageStyles = NULL;
	FMessageWidgets = NULL;
	initialize(APluginManager);

	FViewWidget = FMessageWidgets!=NULL ? FMessageWidgets->newViewWidget(streamJid(),Jid::null,ui.wdtMessages) : NULL;
	if (FViewWidget)
	{
		ui.wdtMessages->setLayout(new QVBoxLayout);
		ui.wdtMessages->layout()->setMargin(0);
		ui.wdtMessages->layout()->addWidget(FViewWidget->instance());
	}
}

ArchiveViewWindow::~ArchiveViewWindow()
{

}

Jid ArchiveViewWindow::streamJid() const
{
	return FRoster->streamJid();
}

Jid ArchiveViewWindow::contactJid() const
{
	return Jid::null;
}

void ArchiveViewWindow::setContactJid(const Jid &AContactJid)
{

}

void ArchiveViewWindow::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
}
