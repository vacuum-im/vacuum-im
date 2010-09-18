#include "shortcuts.h"

#include <QHash>
#include <QVariant>

struct Shortcuts::ShortcutsData
{
	QHash<QString, QString> groups;
	QHash<QString, ShortcutDescriptor> shortcuts;
	QHash<QObject *, QString> objects;
};
Shortcuts::ShortcutsData *Shortcuts::d = new Shortcuts::ShortcutsData;

Shortcuts *Shortcuts::instance()
{
	static Shortcuts *inst = NULL;
	if (inst == NULL)
		inst = new Shortcuts();
	return inst;
}

QList<QString> Shortcuts::groups()
{
	return d->groups.keys();
}

QString Shortcuts::groupDescription( const QString &AId )
{
	return d->groups.value(AId);
}

/**
  Declare a group of shortcuts.

  A group has id of same format as a shortcut, is is expected to be a prefix of
  shprtcuts belonging to this group, e.g.:

  group: messages.tabwindow
  shortcut: messages.tabwindow.close-tab

  It's recommended to declareShortcut all groups a set of shorcuts belong to
  just before declaring these shortcuts.
**/
void Shortcuts::declareGroup(const QString &AId, const QString &ADescription)
{
	if (!AId.isEmpty())
	{
		d->groups.insert(AId,ADescription);
		emit instance()->groupDeclared(AId,ADescription);
	}
}

QList<QString> Shortcuts::shortcuts()
{
	return d->shortcuts.keys();
}

ShortcutDescriptor Shortcuts::shortcutDescriptor(const QString &AId)
{
	return d->shortcuts.value(AId);
}

/**
  Declare shortcut identified by AId and supply default value to it.

  ADescription is user-readable text shown to user in global hotkey settings.
**/
void Shortcuts::declareShortcut(const QString &AId, const QString &ADescription, const QKeySequence &ADefaultKey, Qt::ShortcutContext AContext)
{
	if (!AId.isEmpty() && !ADescription.isEmpty())
	{
		ShortcutDescriptor &descriptor = d->shortcuts[AId];
		descriptor.description = ADescription;
		descriptor.defaultKey = ADefaultKey;
		descriptor.context = AContext;
		emit instance()->shortcutDeclared(AId,descriptor);
	}
}

/**
  Update shortcut binding for id. All objects bound to it will be updated immediately.
**/
void Shortcuts::updateShortcut(const QString &AId, const QKeySequence &AUserKey)
{
	if (d->shortcuts.contains(AId))
	{
		ShortcutDescriptor &descriptor = d->shortcuts[AId];
		descriptor.userKey = AUserKey!=descriptor.defaultKey ? AUserKey : QKeySequence();
		foreach(QObject *object, d->objects.keys(AId)) {
			updateObject(object); }
		emit instance()->shortcutUpdated(AId,descriptor);
	}
}

/**
  Set up a binding between id and object.

  Following QObject properties are used
	QKeySequence        "shortcut"
	Qt::ShortcutContext "shortcutContext"

  QAction supplies both of them, QAbstractButton supplies only "shortcut".
**/
void Shortcuts::bindShortcut(const QString &AId, QObject *AObject)
{
	if (AObject)
	{
		if (!AId.isEmpty())
			d->objects.insert(AObject,AId);
		else 
			d->objects.remove(AObject);
		updateObject(AObject);
		instance()->shortcutBinded(AId,AObject);
	}
}

void Shortcuts::updateObject(QObject *AObject)
{
	QString id = d->objects.value(AObject);
	if (!id.isEmpty())
	{
		ShortcutDescriptor descriptor = d->shortcuts.value(id);
		AObject->setProperty("shortcut", descriptor.userKey.isEmpty() ? descriptor.defaultKey : descriptor.userKey);
		AObject->setProperty("shotcutContext", descriptor.context);
	}
	else if (AObject)
	{
		AObject->setProperty("shortcut",QVariant());
		AObject->setProperty("shotcutContext",QVariant());
	}
}

void Shortcuts::onObjectDestroyed(QObject *AObject)
{
	d->objects.remove(AObject);
}

// ====== Options backend usage

//void Shortcuts::onOptionsOpened()
//{
//	// Load settings
//	//qDebug() << this << "Options opened";
//
//	OptionsNode options = Options::node("shortcuts");
//	for (ShortcutMap::iterator shortcut = d->shortcuts.begin();
//		shortcut != d->shortcuts.end(); shortcut++)
//	{
//		if (options.hasValue(shortcut.key()))
//		{
//			shortcut->FUserSetKeySequence = options.value(shortcut.key()).toString();
//			qDebug() << this << "Loaded from settings:" << shortcut.key();
//		}
//		else
//			shortcut->FUserSetKeySequence = QKeySequence();
//	}
//}
//
//void Shortcuts::onOptionsClosed()
//{
//	// Save settings
//	//qDebug() << this << "Options closing";
//
//	OptionsNode options = Options::node("shortcuts");
//	options.removeChilds(); // Wipe out all old settings
//
//	for (ShortcutMap::const_iterator shortcut = d->shortcuts.constBegin();
//		shortcut != d->shortcuts.constEnd(); shortcut++)
//	{
//		if (shortcut->selectKeySequence() != shortcut->FDefaultKeySequence)
//			options.setValue(shortcut->FUserSetKeySequence.toString(), shortcut.key());
//	}
//}
