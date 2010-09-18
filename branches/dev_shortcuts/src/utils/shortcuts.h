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
	QKeySequence userKey;
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
	static void updateShortcut(const QString &AId, const QKeySequence &AUserKey);
	static void bindShortcut(const QString &AId, QObject *AObject);
signals:
	void groupDeclared(const QString &AId, const QString &ADescription);
	void shortcutDeclared(const QString &AId, const ShortcutDescriptor &ADescriptor);
	void shortcutUpdated(const QString &AId, const ShortcutDescriptor &ADescriptor);
	void shortcutBinded(const QString &AId, QObject *AObject);
protected:
	static void updateObject(QObject *AObject);
protected slots:
	void onObjectDestroyed(QObject *AObject);
private:
	static ShortcutsData *d;
};

#endif // SHORTCUTS_H
