#include "shortcuts.h"

#include <QHash>
#include <QAction>
#include <QVariant>
#include <QApplication>
#include <QDesktopWidget>
#include <thirdparty/qxtglobalshortcut/qxtglobalshortcut.h>

QKeySequence correctKeySequence(const QKeySequence &AKey)
{
#ifdef Q_OS_WIN
	if ((AKey[0] & ~Qt::KeyboardModifierMask) == Qt::Key_Backtab)
		return QKeySequence(Qt::Key_Tab | (AKey[0] & Qt::KeyboardModifierMask));
#endif
	return AKey;
}

struct Shortcuts::ShortcutsData
{
	QHash<QString, QPair<QString,int> > groups;
	QHash<QString, Shortcuts::Descriptor> shortcuts;
	QMap<QObject *, QString> objectShortcutsId;
	QMap<QShortcut *, QString> widgetShortcutsId;
	QMap<QShortcut *, QWidget *> widgetShortcutsWidget;
	QMap<QxtGlobalShortcut *, QString> globalShortcutsId;
};

Shortcuts::Shortcuts()
{
	d = new ShortcutsData;
}

Shortcuts::~Shortcuts()
{
	delete d;
}

Shortcuts *Shortcuts::instance()
{
	static Shortcuts *inst = new Shortcuts();
	return inst;
}

QList<QString> Shortcuts::groups()
{
	return instance()->d->groups.keys();
}

int Shortcuts::groupOrder(const QString &AId)
{
	return instance()->d->groups.value(AId).second;
}

QString Shortcuts::groupDescription(const QString &AId)
{
	return instance()->d->groups.value(AId).first;
}

void Shortcuts::declareGroup(const QString &AId, const QString &ADescription, int AOrder)
{
	if (!AId.isEmpty() && !ADescription.isEmpty())
	{
		instance()->d->groups.insert(AId,qMakePair<QString,int>(ADescription,AOrder));
		emit instance()->groupDeclared(AId);
	}
}

QList<QString> Shortcuts::shortcuts()
{
	return instance()->d->shortcuts.keys();
}

Shortcuts::Descriptor Shortcuts::shortcutDescriptor(const QString &AId)
{
	return instance()->d->shortcuts.value(AId);
}

void Shortcuts::declareShortcut(const QString &AId, const QString &ADescription, const QKeySequence &ADefaultKey, Context AContext)
{
	if (!AId.isEmpty())
	{
		Descriptor &descriptor = instance()->d->shortcuts[AId];
		descriptor.description = ADescription;
		descriptor.defaultKey = ADefaultKey;
		descriptor.activeKey = ADefaultKey;
		descriptor.context = AContext;
		emit instance()->shortcutDeclared(AId);
	}
}

void Shortcuts::updateShortcut(const QString &AId, const QKeySequence &AKey)
{
	ShortcutsData *q = instance()->d;
	if (q->shortcuts.contains(AId))
	{
		Descriptor &descriptor = q->shortcuts[AId];
		descriptor.activeKey = AKey;
		foreach(QObject *object, q->objectShortcutsId.keys(AId))
			updateObject(object);
		foreach(QShortcut *shortcut, q->widgetShortcutsId.keys(AId))
			updateWidget(shortcut);
		foreach(QxtGlobalShortcut *shortcut, q->globalShortcutsId.keys(AId))
			updateGlobal(shortcut);
		emit instance()->shortcutUpdated(AId);
	}
}

QString Shortcuts::objectShortcut(QObject *AObject)
{
	return instance()->d->objectShortcutsId.value(AObject);
}

void Shortcuts::bindObjectShortcut(const QString &AId, QObject *AObject)
{
	if (AObject)
	{
		if (!AId.isEmpty())
		{
			instance()->d->objectShortcutsId.insert(AObject,AId);
			connect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		else 
		{
			instance()->d->objectShortcutsId.remove(AObject);
			disconnect(AObject,SIGNAL(destroyed(QObject *)),instance(),SLOT(onObjectDestroyed(QObject *)));
		}
		updateObject(AObject);
		emit instance()->shortcutBinded(AId,AObject);
	}
}

QList<QString> Shortcuts::widgetShortcuts(QWidget *AWidget)
{
	QList<QString> shortcuts;
	QMap<QShortcut *, QString> shortcutsId = instance()->d->widgetShortcutsId;
	foreach(QShortcut *shortcut, instance()->d->widgetShortcutsWidget.keys(AWidget))
		shortcuts.append(shortcutsId.value(shortcut));
	return shortcuts;
}

void Shortcuts::insertWidgetShortcut(const QString &AId, QWidget *AWidget)
{
	if (AWidget!=NULL && !widgetShortcuts(AWidget).contains(AId))
	{
		ShortcutsData *q = instance()->d;
		if (!q->widgetShortcutsWidget.values().contains(AWidget))
			connect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
		QShortcut *shortcut = new QShortcut(AWidget);
		q->widgetShortcutsId.insert(shortcut, AId);
		q->widgetShortcutsWidget.insert(shortcut, AWidget);
		connect(shortcut,SIGNAL(activated()),instance(),SLOT(onShortcutActivated()));
		connect(shortcut,SIGNAL(activatedAmbiguously()),instance(),SLOT(onShortcutActivated()));
		updateWidget(shortcut);
		emit instance()->shortcutInserted(AId,AWidget);
	}
}

void Shortcuts::removeWidgetShortcut(const QString &AId, QWidget *AWidget)
{
	if (widgetShortcuts(AWidget).contains(AId))
	{
		ShortcutsData *q = instance()->d;
		foreach(QShortcut *shortcut, q->widgetShortcutsWidget.keys(AWidget))
		{
			if (q->widgetShortcutsId.value(shortcut) == AId)
			{
				q->widgetShortcutsId.remove(shortcut);
				q->widgetShortcutsWidget.remove(shortcut);
				delete shortcut;
				if (!q->widgetShortcutsWidget.values().contains(AWidget))
					disconnect(AWidget,SIGNAL(destroyed(QObject *)),instance(),SLOT(onWidgetDestroyed(QObject *)));
				emit instance()->shortcutRemoved(AId,AWidget);
				break;
			}
		}
	}
}

QList<QString> Shortcuts::globalShortcuts()
{
	return instance()->d->globalShortcutsId.values();
}

bool Shortcuts::isGlobalShortcutActive(const QString &AId)
{
	QxtGlobalShortcut *shortcut = instance()->d->globalShortcutsId.key(AId);
	return shortcut!=NULL ? shortcut->isEnabled() : false;
}

void Shortcuts::setGlobalShortcut(const QString &AId, bool AEnabled)
{
	ShortcutsData *q = instance()->d;
	QxtGlobalShortcut *shortcut = q->globalShortcutsId.key(AId);
	if (AEnabled && shortcut==NULL)
	{
		shortcut = new QxtGlobalShortcut(instance());
		q->globalShortcutsId.insert(shortcut,AId);
		connect(shortcut,SIGNAL(activated()),instance(),SLOT(onGlobalShortcutActivated()));
		updateGlobal(shortcut);
		emit instance()->shortcutEnabled(AId, AEnabled);
	}
	else if (!AEnabled && shortcut!=NULL)
	{
		q->globalShortcutsId.remove(shortcut);
		delete shortcut;
		emit instance()->shortcutEnabled(AId, AEnabled);
	}
}

void Shortcuts::activateShortcut(const QString &AId, QWidget *AWidget)
{
	emit instance()->shortcutActivated(AId,AWidget);
}

void Shortcuts::updateObject(QObject *AObject)
{
	static QDesktopWidget *deskWidget = QApplication::desktop();

	QString id = instance()->d->objectShortcutsId.value(AObject);
	if (!id.isEmpty())
	{
		const Descriptor &descriptor = instance()->d->shortcuts.value(id);
		if (descriptor.context == ApplicationShortcut)
		{
			QAction *action = qobject_cast<QAction *>(AObject);
			if (action && !deskWidget->actions().contains(action))
				deskWidget->addAction(action);
		}
		AObject->setProperty("shortcut", correctKeySequence(descriptor.activeKey));
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
	Descriptor descriptor = instance()->d->shortcuts.value(instance()->d->widgetShortcutsId.value(AShortcut));
	AShortcut->setKey(correctKeySequence(descriptor.activeKey));
	AShortcut->setContext(convertContext(descriptor.context));
}

void Shortcuts::updateGlobal(QxtGlobalShortcut *AShortcut)
{
	Descriptor descriptor = instance()->d->shortcuts.value(instance()->d->globalShortcutsId.value(AShortcut));
	if (!descriptor.activeKey.isEmpty())
	{
		bool registered = AShortcut->setShortcut(descriptor.activeKey);
		AShortcut->setEnabled(registered);
	}
	else if (!AShortcut->shortcut().isEmpty())
	{
		AShortcut->setShortcut(QKeySequence());
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
