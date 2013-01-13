#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QObject>
#include <QShortcut>
#include <QKeySequence>
#include "utilsexport.h"

class QxtGlobalShortcut;

class UTILS_EXPORT Shortcuts : 
	public QObject
{
	Q_OBJECT;
	struct ShortcutsData;
public:
	enum Context {
		WidgetShortcut,
		WindowShortcut,
		ApplicationShortcut,
		WidgetWithChildrenShortcut,
		GlobalShortcut
	};
	struct Descriptor {
		Descriptor() {
			context = WindowShortcut;
		}
		QKeySequence activeKey;
		QKeySequence defaultKey;
		Context context;
		QString description;
	};
public:
	static Shortcuts *instance();
	static QList<QString> groups();
	static QString groupDescription(const QString &AId);
	static void declareGroup(const QString &AId, const QString &ADescription);
	static QList<QString> shortcuts();
	static Descriptor shortcutDescriptor(const QString &AId);
	static void declareShortcut(const QString &AId, const QString &ADescription, const QKeySequence &ADefaultKey, Context AContext = WindowShortcut);
	static void updateShortcut(const QString &AId, const QKeySequence &AKey);
	static QString objectShortcut(QObject *AObject);
	static void bindObjectShortcut(const QString &AId, QObject *AObject);
	static QList<QString> widgetShortcuts(QWidget *AWidget);
	static void insertWidgetShortcut(const QString &AId, QWidget *AWidget);
	static void removeWidgetShortcut(const QString &AId, QWidget *AWidget);
	static QList<QString> globalShortcuts();
	static void setGlobalShortcut(const QString &AId, bool AEnabled);
signals:
	void groupDeclared(const QString &AId);
	void shortcutDeclared(const QString &AId);
	void shortcutUpdated(const QString &AId);
	void shortcutBinded(const QString &AId, QObject *AObject);
	void shortcutInserted(const QString &AId, QWidget *AWidget);
	void shortcutRemoved(const QString &AId, QWidget *AWidget);
	void shortcutEnabled(const QString &AId, bool AEnabled);
	void shortcutActivated(const QString &AId, QWidget *AWidget);
protected:
	static void updateObject(QObject *AObject);
	static void updateWidget(QShortcut *AShortcut);
	static void updateGlobal(QxtGlobalShortcut *AShortcut);
	static Qt::ShortcutContext convertContext(Context AContext);
protected slots:
	void onShortcutActivated();
	void onGlobalShortcutActivated();
	void onWidgetDestroyed(QObject *AObject);
	void onObjectDestroyed(QObject *AObject);
private:
	static ShortcutsData *d;
};

#endif // SHORTCUTS_H
