#include "shortcuts.h"

#include <QHash>
#include <QAction>
#include <QVariant>
#include <QApplication>
#include <QDesktopWidget>
#include <thirdparty/qxtglobalshortcut/qxtglobalshortcut.h>

struct Shortcuts::ShortcutsData
{
	QHash<QString, QString> groups;
	QHash<QString, Descriptor> shortcuts;
	QMap<QObject *, QString> objectShortcutsId;
	QMap<QShortcut *, QString> widgetShortcutsId;
	QMap<QShortcut *, QWidget *> widgetShortcutsWidget;
	QMap<QxtGlobalShortcut *, QString> globalShortcutsId;
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

Shortcuts::Descriptor Shortcuts::shortcutDescriptor(const QString &AId)
{
	return d->shortcuts.value(AId);
}

void Shortcuts::declareShortcut(const QString &AId, const QString &ADescription, const QKeySequence &ADefaultKey, Context AContext)
{
	if (!AId.isEmpty())
	{
		Descriptor &descriptor = d->shortcuts[AId];
		descriptor.description = ADescription;
		descriptor.defaultKey = ADefaultKey;
		descriptor.activeKey = ADefaultKey;
		descriptor.context = AContext;
		emit instance()->shortcutDeclared(AId);
	}
}

void Shortcuts::updateShortcut(const QString &AId, const QKeySequence &AKey)
{
	if (d->shortcuts.contains(AId))
	{
		Descriptor &descriptor = d->shortcuts[AId];
		descriptor.activeKey = AKey;
		foreach(QObject *object, d->objectShortcutsId.keys(AId)) {
			updateObject(object); }
		foreach(QShortcut *shortcut, d->widgetShortcutsId.keys(AId)) {
			updateWidget(shortcut); }
		foreach(QxtGlobalShortcut *shortcut, d->globalShortcutsId.keys(AId)) {
			updateGlobal(shortcut); }
		emit instance()->shortcutUpdated(AId);
	}
}

QString Shortcuts::objectShortcut(QObject *AObject)
{
	return d->objectShortcutsId.value(AObject);
}

void Shortcuts::bindObjectShortcut(const QString &AId, QObject *AObject)
{
	if (AObject)
	{
		if (!AId.isEmpty())
		{
			d->objectShortcutsId.insert(AObject,AId);
			connect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		else 
		{
			d->objectShortcutsId.remove(AObject);
			disconnect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		updateObject(AObject);
		emit instance()->shortcutBinded(AId,AObject);
	}
}

QList<QString> Shortcuts::widgetShortcuts(QWidget *AWidget)
{
	QList<QString> shortcuts;
	foreach(QShortcut *shortcut, d->widgetShortcutsWidget.keys(AWidget))
		shortcuts.append(d->widgetShortcutsId.value(shortcut));
	return shortcuts;
}

void Shortcuts::insertWidgetShortcut(const QString &AId, QWidget *AWidget)
{
	if (AWidget!=NULL && !widgetShortcuts(AWidget).contains(AId))
	{
		if (!d->widgetShortcutsWidget.values().contains(AWidget))
			connect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
		QShortcut *shortcut = new QShortcut(AWidget);
		d->widgetShortcutsId.insert(shortcut, AId);
		d->widgetShortcutsWidget.insert(shortcut, AWidget);
		connect(shortcut,SIGNAL(activated()),instance(),SLOT(onShortcutActivated()));
		updateWidget(shortcut);
		emit instance()->shortcutInserted(AId,AWidget);
	}
}

void Shortcuts::removeWidgetShortcut(const QString &AId, QWidget *AWidget)
{
	if (widgetShortcuts(AWidget).contains(AId))
	{
		foreach(QShortcut *shortcut, d->widgetShortcutsWidget.keys(AWidget))
		{
			if (d->widgetShortcutsId.value(shortcut) == AId)
			{
				d->widgetShortcutsId.remove(shortcut);
				d->widgetShortcutsWidget.remove(shortcut);
				delete shortcut;
				if (!d->widgetShortcutsWidget.values().contains(AWidget))
					disconnect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
				emit instance()->shortcutRemoved(AId,AWidget);
				break;
			}
		}
	}
}

QList<QString> Shortcuts::globalShortcuts()
{
	return d->globalShortcutsId.values();
}

void Shortcuts::setGlobalShortcut(const QString &AId, bool AEnabled)
{
	QxtGlobalShortcut *shortcut = d->globalShortcutsId.key(AId);
	if (AEnabled && shortcut==NULL)
	{
		shortcut = new QxtGlobalShortcut(instance());
		d->globalShortcutsId.insert(shortcut,AId);
		connect(shortcut,SIGNAL(activated()),instance(),SLOT(onGlobalShortcutActivated()));
		updateGlobal(shortcut);
		emit instance()->shortcutEnabled(AId, AEnabled);
	}
	else if (!AEnabled && shortcut!=NULL)
	{
		d->globalShortcutsId.remove(shortcut);
		delete shortcut;
		emit instance()->shortcutEnabled(AId, AEnabled);
	}
}

void Shortcuts::updateObject(QObject *AObject)
{
	static QDesktopWidget *deskWidget = QApplication::desktop();

	QString id = d->objectShortcutsId.value(AObject);
	if (!id.isEmpty())
	{
		const Descriptor &descriptor = d->shortcuts.value(id);
		if (descriptor.context == ApplicationShortcut)
		{
			QAction *action = qobject_cast<QAction *>(AObject);
			if (action && !deskWidget->actions().contains(action))
				deskWidget->addAction(action);
		}
		AObject->setProperty("shortcut", descriptor.activeKey);
		AObject->setProperty("shortcutContext", convertContext(descriptor.context));
	}
	else if (AObject)
	{
		if (AObject->property("shortcutContext").toInt() == Qt::ApplicationShortcut)
		{
			QAction *action = qobject_cast<QAction *>(AObject);
			if (action)
				deskWidget->removeAction(action);
		}
		AObject->setProperty("shortcut",QVariant());
		AObject->setProperty("shortcutContext",QVariant());
	}
}

void Shortcuts::updateWidget(QShortcut *AShortcut)
{
	Descriptor descriptor = d->shortcuts.value(d->widgetShortcutsId.value(AShortcut));
	AShortcut->setKey(descriptor.activeKey);
	AShortcut->setContext(convertContext(descriptor.context));
}

void Shortcuts::updateGlobal(QxtGlobalShortcut *AShortcut)
{
	Descriptor descriptor = d->shortcuts.value(d->globalShortcutsId.value(AShortcut));
	if (!descriptor.activeKey.isEmpty())
	{
		AShortcut->setShortcut(descriptor.activeKey);
		AShortcut->setEnabled(true);
	}
	else
	{
		AShortcut->setEnabled(false);
	}
}

Qt::ShortcutContext Shortcuts::convertContext(Context AContext)
{
	switch (AContext)
	{
	case WidgetShortcut:
		return Qt::WidgetShortcut;
	case WindowShortcut:
		return Qt::WindowShortcut;
	case ApplicationShortcut:
		return Qt::ApplicationShortcut;
	case WidgetWithChildrenShortcut:
		return Qt::WidgetWithChildrenShortcut;
	case GlobalShortcut:
		return Qt::ApplicationShortcut;
	default:
		return Qt::WindowShortcut;
	}
	return Qt::WindowShortcut;
}

void Shortcuts::onShortcutActivated()
{
	QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
	if (shortcut)
		emit instance()->shortcutActivated(d->widgetShortcutsId.value(shortcut), d->widgetShortcutsWidget.value(shortcut));
}

void Shortcuts::onGlobalShortcutActivated()
{
	QxtGlobalShortcut *shortcut = qobject_cast<QxtGlobalShortcut *>(sender());
	if (shortcut)
		emit instance()->shortcutActivated(d->globalShortcutsId.value(shortcut),NULL);
}

void Shortcuts::onWidgetDestroyed(QObject *AObject)
{
	foreach(QWidget *widget, d->widgetShortcutsWidget.values())
	{
		if (qobject_cast<QObject *>(widget) == AObject)
		{
			foreach(QShortcut *shortcut, d->widgetShortcutsWidget.keys(widget))
			{
				QString id = d->widgetShortcutsId.take(shortcut);
				d->widgetShortcutsWidget.remove(shortcut);
				emit instance()->shortcutRemoved(id,widget);
			}
			break;
		}
	}
}

void Shortcuts::onObjectDestroyed(QObject *AObject)
{
	d->objectShortcutsId.remove(AObject);
}
