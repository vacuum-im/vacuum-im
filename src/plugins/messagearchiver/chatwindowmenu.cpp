#include "chatwindowmenu.h"

#include <definitions/namespaces.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <utils/pluginhelper.h>
#include <utils/menu.h>

#define SFP_LOGGING           "logging"
#define SFV_MUSTNOT_LOGGING   "mustnot"

ChatWindowMenu::ChatWindowMenu(IMessageArchiver *AArchiver, IMessageToolBarWidget *AToolBarWidget, QWidget *AParent) : Menu(AParent)
{
	FArchiver = AArchiver;
	connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &)),SLOT(onArchivePrefsChanged(const Jid &)));
	connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),SLOT(onArchiveRequestCompleted(const QString &)));
	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));

	FToolBarWidget = AToolBarWidget;
	connect(FToolBarWidget->messageWindow()->address()->instance(),SIGNAL(addressChanged(const Jid &,const Jid &)),SLOT(onToolBarWidgetAddressChanged(const Jid &,const Jid &)));
	
	createActions();
	updateMenu();
}

ChatWindowMenu::~ChatWindowMenu()
{

}

Jid ChatWindowMenu::streamJid() const
{
	return FToolBarWidget->messageWindow()->address()->streamJid();
}

Jid ChatWindowMenu::contactJid() const
{
	return FToolBarWidget->messageWindow()->address()->contactJid();
}

void ChatWindowMenu::createActions()
{
	FEnableArchiving = new Action(this);
	FEnableArchiving->setCheckable(true);
	FEnableArchiving->setText(tr("Enable Message Archiving"));
	connect(FEnableArchiving,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FEnableArchiving,AG_DEFAULT,false);

	FDisableArchiving = new Action(this);
	FDisableArchiving->setCheckable(true);
	FDisableArchiving->setText(tr("Disable Message Archiving"));
	connect(FDisableArchiving,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FDisableArchiving,AG_DEFAULT,false);
}

void ChatWindowMenu::updateMenu()
{
	if (FArchiver->isPrefsSupported(streamJid()))
	{
		FEnableArchiving->setEnabled(FSaveRequest.isEmpty());
		FEnableArchiving->setChecked(FArchiver->isArchivingAllowed(streamJid(), contactJid()));

		FDisableArchiving->setEnabled(FSaveRequest.isEmpty());
		FDisableArchiving->setChecked(!FArchiver->isArchivingAllowed(streamJid(), contactJid()));
	}
	else
	{
		FEnableArchiving->setEnabled(false);
		FEnableArchiving->setChecked(false);
		
		FDisableArchiving->setEnabled(false);
		FDisableArchiving->setChecked(false);
	}
}

void ChatWindowMenu::onActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action && FSaveRequest.isEmpty())
	{
		if (action == FEnableArchiving)
		{
			IArchiveStreamPrefs prefs = FArchiver->archivePrefs(streamJid());
			prefs.never -= contactJid().bare();
			prefs.always += contactJid().bare();
			FSaveRequest = FArchiver->setArchivePrefs(streamJid(),prefs);
		}
		else if (action == FDisableArchiving)
		{
			IArchiveStreamPrefs prefs = FArchiver->archivePrefs(streamJid());
			prefs.always -= contactJid().bare();
			prefs.never += contactJid().bare();
			FSaveRequest = FArchiver->setArchivePrefs(streamJid(),prefs);
		}
		updateMenu();
	}
}

void ChatWindowMenu::onArchivePrefsChanged(const Jid &AStreamJid)
{
	if (streamJid() == AStreamJid)
		updateMenu();
}

void ChatWindowMenu::onArchiveRequestCompleted(const QString &AId)
{
	if (FSaveRequest == AId)
	{
		FSaveRequest.clear();
		updateMenu();
	}
}

void ChatWindowMenu::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FSaveRequest == AId)
	{
		if (FToolBarWidget->messageWindow()->viewWidget() != NULL)
		{
			IMessageStyleContentOptions options;
			options.kind = IMessageStyleContentOptions::KindStatus;
			options.type |= IMessageStyleContentOptions::TypeEvent;
			options.direction = IMessageStyleContentOptions::DirectionIn;
			options.time = QDateTime::currentDateTime();
			FToolBarWidget->messageWindow()->viewWidget()->appendText(tr("Failed to change archive preferences: %1").arg(AError.errorMessage()),options);
		}

		FSaveRequest.clear();
		updateMenu();
	}
}

void ChatWindowMenu::onToolBarWidgetAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	updateMenu();
}
