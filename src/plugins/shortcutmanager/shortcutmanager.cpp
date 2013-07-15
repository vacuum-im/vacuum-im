#include "shortcutmanager.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>

ShortcutManager::ShortcutManager()
{
	FTrayManager = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;

	FAllHidden = false;
	FTrayHidden = false;
	FNotifyHidden = 0;
}

ShortcutManager::~ShortcutManager()
{

}

void ShortcutManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Shortcut Manager");
	APluginInfo->description = tr("Allows to setup user defined shortcuts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(OPTIONSMANAGER_UUID);
}

bool ShortcutManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
		FNotifications = qobject_cast<INotifications *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	return FOptionsManager!=NULL;
}

bool ShortcutManager::initObjects()
{
	Shortcuts::declareShortcut(SCT_GLOBAL_HIDEALLWIDGETS,tr("Hide all windows, tray icon and notifications"),QKeySequence::UnknownKey,Shortcuts::GlobalShortcut);
	return true;
}

bool ShortcutManager::initSettings()
{
	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_SHORTCUTS, OPN_SHORTCUTS, tr("Shortcuts"), MNI_SHORTCUTS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

bool ShortcutManager::startPlugin()
{
	Shortcuts::setGlobalShortcut(SCT_GLOBAL_HIDEALLWIDGETS,true);
	return true;
}

QMultiMap<int, IOptionsWidget *> ShortcutManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_SHORTCUTS)
	{
		widgets.insertMulti(OWO_SHORTCUTS, new ShortcutOptionsWidget(AParent));
	}
	return widgets;
}

void ShortcutManager::hideAllWidgets()
{
	if (FOptionsManager==NULL || FOptionsManager->isOpened())
	{
		foreach(QWidget *widget, QApplication::topLevelWidgets())
		{
			if (!widget->isHidden())
			{
				QPointer<QWidget> pwidget = widget;
				widget->hide();
				FHiddenWidgets.append(pwidget);
			}
		}
		if (FTrayManager && FTrayManager->isTrayIconVisible())
		{
			FTrayHidden = true;
			FTrayManager->setTrayIconVisible(false);
		}
		if (FNotifications)
		{
			FNotifyHidden = 0;
			if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::PopupWindow)).value().toBool())
			{
				FNotifyHidden |= INotification::PopupWindow;
				Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::PopupWindow)).setValue(false);
			}
			if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::SoundPlay)).value().toBool())
			{
				FNotifyHidden |= INotification::SoundPlay;
				Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::SoundPlay)).setValue(false);
			}
			if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AlertWidget)).value().toBool())
			{
				FNotifyHidden |= INotification::AlertWidget;
				Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AlertWidget)).setValue(false);
			}
			if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::ShowMinimized)).value().toBool())
			{
				FNotifyHidden |= INotification::ShowMinimized;
				Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::ShowMinimized)).setValue(false);
			}
			if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AutoActivate)).value().toBool())
			{
				FNotifyHidden |= INotification::AutoActivate;
				Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AutoActivate)).setValue(false);
			}
		}
		FAllHidden = true;
	}
}

void ShortcutManager::showHiddenWidgets(bool ACheckPassword)
{
	static bool blocked = false;
	if (!blocked)
	{
		blocked = true;

		QString password;
		QString profile = FOptionsManager!=NULL ? FOptionsManager->currentProfile() : QString::null;
		QString title = QString("%1 - %2").arg(CLIENT_NAME).arg(profile);

		if (ACheckPassword && FOptionsManager!=NULL && FOptionsManager->isOpened() && !FOptionsManager->checkProfilePassword(profile,password))
		{
			password = QInputDialog::getText(NULL,title,tr("Enter profile password:"),QLineEdit::Password);
		}

		if (!ACheckPassword || FOptionsManager==NULL || FOptionsManager->checkProfilePassword(profile,password))
		{
			foreach(const QPointer<QWidget> &pwidget, FHiddenWidgets)
			{
				if (!pwidget.isNull())
					pwidget->show();
			}
			if (FTrayManager && FTrayHidden)
			{
				FTrayHidden = false;
				FTrayManager->setTrayIconVisible(true);
			}
			if (FNotifications)
			{
				if (FNotifyHidden & INotification::PopupWindow)
				{
					Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::PopupWindow)).setValue(true);
				}
				if (FNotifyHidden & INotification::SoundPlay)
				{
					Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::SoundPlay)).setValue(true);
				}
				if (FNotifyHidden & INotification::AlertWidget)
				{
					Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AlertWidget)).setValue(true);
				}
				if (FNotifyHidden & INotification::ShowMinimized)
				{
					Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::ShowMinimized)).setValue(true);
				}
				if (FNotifyHidden & INotification::AutoActivate)
				{
					Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AutoActivate)).setValue(true);
				}
				FNotifyHidden = 0;
			}
			FHiddenWidgets.clear();
			FAllHidden = false;
		}
		else if (!password.isEmpty())
		{
			QMessageBox::critical(NULL,title,tr("Wrong profile password!"));
		}

		blocked = false;
	}
}

void ShortcutManager::onOptionsOpened()
{
	OptionsNode options = Options::node(OPV_SHORTCUTS);
	foreach(QString shortcutId, Shortcuts::shortcuts())
	{
		if (options.hasNode(shortcutId))
		{
			Shortcuts::updateShortcut(shortcutId,options.value(shortcutId).toString());
		}
	}
}

void ShortcutManager::onOptionsClosed()
{
	if (FAllHidden)
		showHiddenWidgets(false);

	OptionsNode options = Options::node(OPV_SHORTCUTS);
	foreach(QString shortcutId, Shortcuts::shortcuts())
	{
		Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcutId);
		if (descriptor.activeKey != descriptor.defaultKey)
			options.setValue(descriptor.activeKey.toString(),shortcutId);
		else
			options.removeNode(shortcutId);
	}
}

void ShortcutManager::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_GLOBAL_HIDEALLWIDGETS && AWidget==NULL)
	{
		if (!FAllHidden)
			hideAllWidgets();
		else
			showHiddenWidgets();
	}
}

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN2(plg_shortcutmanager, ShortcutManager)
#endif
