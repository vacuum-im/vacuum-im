#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QObject>
#include <QPair>
#include <QList>
#include <QScopedPointer>
#include <QPointer>
#include <QKeySequence>
#include "utilsexport.h"

struct ShortcutsPrivate;

class UTILS_EXPORT Shortcuts : public QObject
{
	Q_OBJECT;
public:
	/**
	  Shortcut data as it is stored. The only way to access this data is to
	  obtain a const-reference to shortcuts map.

	  Consider data fields read-only.
	  **/
	class ShortcutData
	{
	public:
		QString FDescription;
		QKeySequence FDefaultKeySequence;
		QKeySequence FUserSetKeySequence;
		Qt::ShortcutContext FContext;

	private:
		QList<QPointer<QObject> > FBoundObjects;

		ShortcutData(const QString &ADescription,
					 const QKeySequence &ADefaultKeySequence,
					 Qt::ShortcutContext AContext);

		inline QKeySequence selectKeySequence() const;

		friend struct ShortcutsPrivate;
		friend class Shortcuts;
	};

	/**
	  Shortcut id is stored implicitly as hash index, i.e.
	  <index> -> <data>
	  **/
	typedef QHash<QString, ShortcutData> ShortcutMap;
	/**
	  Groups are stored as
	  <index> -> <displayed name>
	  **/
	typedef QHash<QString, QString> GroupMap;

	static Shortcuts *instance();


	/** Settings Consumer API **/

	/**
	  Declare a group of shortcuts.

	  A group has id of same format as a shortcut, is is expected to be a prefix of
	  shprtcuts belonging to this group, e.g.:

	  group: messages.tabwindow
	  shortcut: messages.tabwindow.close-tab

	  Redeclaring a group is a no-op. Once declared, group's description is not changed.

	  It's recommended to declare all groups a set of shorcuts belong to
	  just before declaring these shortcuts.
	  **/
	static void declareGroup(const QString &AId, const QString &ADescription);
	/**
	  Declare shortcut identified by AId and supply default value to it.

	  ADescription is user-readable text shown to user in global hotkey settings.

	  First, standard key specified by AStandardKey is tried. If it gives
	  an empty key sequence, then ALiteralKey is used.

	  Redeclaring a shortcut is considered an error and will issue a warning.
	  **/
	static void declare(const QString &AId, const QString &ADescription, const QString &ALiteralKey,
		QKeySequence::StandardKey AStandardKey = QKeySequence::UnknownKey,
		Qt::ShortcutContext AContext = Qt::WindowShortcut);
	/**
	  Convenience function for cases when default shortcut is not defined
	  **/
	static void declare(const QString &AId, const QString &ADescription,
						Qt::ShortcutContext AContext = Qt::WindowShortcut);
	/**
	  Set up a binding between id and object.

	  Following QObject properties are used
		QKeySequence        "shortcut"
		Qt::ShortcutContext "shortcutContext"

	  QAction supplies both of them, QAbstractButton supplies only "shortcut".
	  **/
	static void bind(const QString &AId, QObject *AObject);


	/** Settings Provider API **/

	/**
	  Update shortcut binding for id. All objects bound to it will be updated immediately.
	  **/
	static void update(const QString &AId, const QKeySequence &ANewKey);


	/** Introspector API **/

	/**
	  Get all valid shortcut bindings.
	  **/
	static const ShortcutMap shortcutMap();
	/**
	  Get all shortcut groups.
	  **/
	static const GroupMap groupMap();
private slots:
	void onOptionsOpened();
	void onOptionsClosed();
private:
	Shortcuts();
	virtual ~Shortcuts();
	QScopedPointer<ShortcutsPrivate> d;
};

#endif // SHORTCUTS_H
