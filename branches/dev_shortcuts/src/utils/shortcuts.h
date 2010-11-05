#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QObject>
#include <QKeySequence>
#include "utilsexport.h"

struct ShortcutDescriptor
{
	ShortcutDescriptor() {
		context = Qt::WindowShortcut;
	}
	QKeySequence activeKey;
	QKeySequence defaultKey;
	Qt::ShortcutContext context;
	QString description;
};

class UTILS_EXPORT Shortcuts : 
	public QObject
{
	Q_OBJECT;
	struct ShortcutsData;
public:
	static Shortcuts *instance();
	static QList<QString> groups();
	static QString groupDescription(const QString &AId);
	static void declareGroup(const QString &AId, const QString &ADescription);
	static QList<QString> shortcuts();
	static ShortcutDescriptor shortcutDescriptor(const QString &AId);
	static void declareShortcut(const QString &AId, const QString &ADescription, const QKeySequence &ADefaultKey, Qt::ShortcutContext AContext = Qt::WindowShortcut);
	static void updateShortcut(const QString &AId, const QKeySequence &AKey);
	static QString objectShortcut(QObject *AObject);
	static void bindShortcut(const QString &AId, QObject *AObject);
	static QList<QString> widgetShortcuts(QWidget *AWidget);
	static void insertShortcut(const QString &AId, QWidget *AWidget);
	static void removeShortcut(const QString &AId, QWidget *AWidget);
signals:
	void groupDeclared(const QString &AId);
	void shortcutDeclared(const QString &AId);
	void shortcutUpdated(const QString &AId);
	void shortcutBinded(const QString &AId, QObject *AObject);
	void shortcutInserted(const QString &AId, QWidget *AWidget);
	void shortcutActivated(const QString &AId, QWidget *AWidget);
	void shortcutRemoved(const QString &AId, QWidget *AWidget);
protected:
	static void updateObject(QObject *AObject);
	static void updateWidget(QShortcut *AShortcut);
protected slots:
	void onShortcutActivated();
	void onWidgetDestroyed(QObject *AObject);
	void onObjectDestroyed(QObject *AObject);
private:
	static ShortcutsData *d;
};

#endif // SHORTCUTS_H
