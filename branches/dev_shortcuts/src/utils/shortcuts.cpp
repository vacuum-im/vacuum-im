#include "shortcuts.h"

#include <QHash>
#include <QVariant>
#include <QShortcut>

struct Shortcuts::ShortcutsData
{
	QHash<QString, QString> groups;
	QHash<QString, ShortcutDescriptor> shortcuts;
	QHash<QObject *, QString> objects;
	QHash<QShortcut *, QString> qshortcutIds;
	QHash<QShortcut *, QWidget *> qshortcutWidgets;
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
	if (!AId.isEmpty() && !ADescription.isEmpty())
	{
		d->groups.insert(AId,ADescription);
		emit instance()->groupDeclared(AId);
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
	if (!AId.isEmpty())
	{
		ShortcutDescriptor &descriptor = d->shortcuts[AId];
		descriptor.description = ADescription;
		descriptor.defaultKey = ADefaultKey;
		descriptor.activeKey = ADefaultKey;
		descriptor.context = AContext;
		emit instance()->shortcutDeclared(AId);
	}
}

/**
  Update shortcut binding for id. All objects bound to it will be updated immediately.
**/
void Shortcuts::updateShortcut(const QString &AId, const QKeySequence &AKey)
{
	if (d->shortcuts.contains(AId))
	{
		ShortcutDescriptor &descriptor = d->shortcuts[AId];
		descriptor.activeKey = AKey;
		foreach(QObject *object, d->objects.keys(AId)) {
			updateObject(object); }
		foreach(QShortcut *shortcut, d->qshortcutIds.keys(AId)) {
			updateWidget(shortcut); }
		emit instance()->shortcutUpdated(AId);
	}
}

QString Shortcuts::objectShortcut(QObject *AObject)
{
	return d->objects.value(AObject);
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
		{
			d->objects.insert(AObject,AId);
			connect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		else 
		{
			d->objects.remove(AObject);
			disconnect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		updateObject(AObject);
		emit instance()->shortcutBinded(AId,AObject);
	}
}

QList<QString> Shortcuts::widgetShortcuts(QWidget *AWidget)
{
	QList<QString> shortcuts;
	foreach(QShortcut *shortcut, d->qshortcutWidgets.keys(AWidget))
		shortcuts.append(d->qshortcutIds.value(shortcut));
	return shortcuts;
}

void Shortcuts::insertShortcut(const QString &AId, QWidget *AWidget)
{
	if (AWidget!=NULL && !widgetShortcuts(AWidget).contains(AId))
	{
		if (!d->qshortcutWidgets.values().contains(AWidget))
			connect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
		QShortcut *shortcut = new QShortcut(AWidget);
		d->qshortcutIds.insert(shortcut, AId);
		d->qshortcutWidgets.insert(shortcut, AWidget);
		connect(shortcut,SIGNAL(activated()),instance(),SLOT(onShortcutActivated()));
		updateWidget(shortcut);
		emit instance()->shortcutInserted(AId,AWidget);
	}
}

void Shortcuts::removeShortcut(const QString &AId, QWidget *AWidget)
{
	if (widgetShortcuts(AWidget).contains(AId))
	{
		foreach(QShortcut *shortcut, d->qshortcutWidgets.keys(AWidget))
		{
			if (d->qshortcutIds.value(shortcut) == AId)
			{
				d->qshortcutIds.remove(shortcut);
				d->qshortcutWidgets.remove(shortcut);
				delete shortcut;
				if (!d->qshortcutWidgets.values().contains(AWidget))
					disconnect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
				emit instance()->shortcutRemoved(AId,AWidget);
				break;
			}
		}
	}
}

void Shortcuts::updateObject(QObject *AObject)
{
	QString id = d->objects.value(AObject);
	if (!id.isEmpty())
	{
		ShortcutDescriptor descriptor = d->shortcuts.value(id);
		AObject->setProperty("shortcut", descriptor.activeKey);
		AObject->setProperty("shortcutContext", descriptor.context);
	}
	else if (AObject)
	{
		AObject->setProperty("shortcut",QVariant());
		AObject->setProperty("shortcutContext",QVariant());
	}
}

void Shortcuts::updateWidget(QShortcut *AShortcut)
{
	ShortcutDescriptor descriptor = d->shortcuts.value(d->qshortcutIds.value(AShortcut));
	AShortcut->setKey(descriptor.activeKey);
	AShortcut->setContext(descriptor.context);
}

void Shortcuts::onShortcutActivated()
{
	QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
	if (shortcut)
		emit instance()->shortcutActivated(d->qshortcutIds.value(shortcut), d->qshortcutWidgets.value(shortcut));
}

void Shortcuts::onWidgetDestroyed(QObject *AObject)
{
	foreach(QWidget *widget, d->qshortcutWidgets.values())
	{
		if (qobject_cast<QObject *>(widget) == AObject)
		{
			foreach(QShortcut *shortcut, d->qshortcutWidgets.keys(widget))
			{
				QString id = d->qshortcutIds.take(shortcut);
				d->qshortcutWidgets.remove(shortcut);
				emit instance()->shortcutRemoved(id,widget);
			}
			break;
		}
	}
}

void Shortcuts::onObjectDestroyed(QObject *AObject)
{
	d->objects.remove(AObject);
}
