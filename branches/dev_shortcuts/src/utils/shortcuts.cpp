#include "shortcuts.h"
#include "options.h"

#include <QHash>
#include <QPointer>

#include <QDebug>

struct ShortcutsPrivate
{
	Shortcuts::ShortcutMap FShortcuts;
	Shortcuts::GroupMap FGroups;

	// Internal functions
	void applySettings(Shortcuts::ShortcutMap::iterator AShortcut, QObject *AObject);
	void updateBoundObjects(Shortcuts::ShortcutMap::iterator AShortcut);

	// Shortcuts API
	void declareGroup(const QString &AId, const QString &ADescription);
	void declare(const QString &AId, const QString &ADescription,
				 const QString &ALiteralKey, QKeySequence::StandardKey AStandardKey,
				 Qt::ShortcutContext AContext);
	void bind(const QString &AId, QObject *AObject);

	void update(const QString &AId, const QKeySequence &ANewKey);
};


Shortcuts::ShortcutData::ShortcutData(const QString &ADescription,
		const QKeySequence &ADefaultKeySequence,
		Qt::ShortcutContext AContext)
	: FDescription(ADescription),
	  FDefaultKeySequence(ADefaultKeySequence),
	  FContext(AContext)
{
}

inline QKeySequence Shortcuts::ShortcutData::selectKeySequence() const
{
	if (FUserSetKeySequence.isEmpty())
		return FDefaultKeySequence;
	else
		return FUserSetKeySequence;
}


/**
  \internal
  Apply options' contents to object
  **/
void ShortcutsPrivate::applySettings(Shortcuts::ShortcutMap::iterator AShortcut, QObject *AObject)
{
	//qDebug() << "ShortcutsData::applyOptions" << AShortcut.key() << AShortcut->selectKeySequence() << "->" << AObject;

	AObject->setProperty("shortcut", AShortcut->selectKeySequence());
	AObject->setProperty("shotcutContext", AShortcut->FContext);
}


/**
  \internal
  Apply options to all objects bound with id
  **/
void ShortcutsPrivate::updateBoundObjects(Shortcuts::ShortcutMap::iterator AShortcut)
{
	qDebug() << "ShortcutsData::updateShortcut" << AShortcut.key();

	foreach(QPointer<QObject> object, AShortcut->FBoundObjects)
		if (!object.isNull())
			applySettings(AShortcut, object);
}

// ====== Shortcuts public API implementation

void ShortcutsPrivate::declareGroup(const QString &AId, const QString &ADescription)
{
	// FIXME: does not support runtime language change
	Shortcuts::GroupMap::iterator group = FGroups.find(AId);
	if (!FGroups.contains(AId))
		FGroups.insert(AId, ADescription);
}

void ShortcutsPrivate::declare(const QString &AId, const QString &ADescription, const QString &ALiteralKey,
						QKeySequence::StandardKey AStandardKey, Qt::ShortcutContext AContext)
{
	QKeySequence key(AStandardKey);
	if (key.isEmpty())
		key = QKeySequence(ALiteralKey);

	//qDebug() << "Shortcuts::declare" << AId << key << "<-" << ALiteralKey << AStandardKey;

	if (!FShortcuts.contains(AId))
		FShortcuts.insert(AId, Shortcuts::ShortcutData(ADescription, key, AContext));
	else
		qWarning() << this << "Trying to redeclare already declared shortcut" << AId;
}

void ShortcutsPrivate::update(const QString &AId, const QKeySequence &FNewKey)
{
	//qDebug() << "Shortcuts::setKey" << AId << FNewKey;

	Shortcuts::ShortcutMap::iterator shortcut = FShortcuts.find(AId);
	if (shortcut != FShortcuts.end())
		shortcut->FUserSetKeySequence = FNewKey;
	else
		qWarning() << this << "Trying to update undeclared shortcut" << AId;

	updateBoundObjects(shortcut);
}

void ShortcutsPrivate::bind(const QString &AId, QObject *AObject)
{
	if (AObject == NULL)
	{
		qWarning() << this << "Trying to bind" << AId << "to NULL object";
		return;
	}

	//qDebug() << "Shortcuts::bind" << AId << "to" << AObject;

	Shortcuts::ShortcutMap::iterator shortcut = FShortcuts.find(AId);
	if (shortcut != FShortcuts.end())
	{
		shortcut->FBoundObjects << AObject;
		applySettings(shortcut, AObject);
	}
	else
		qWarning() << this << "Trying to bind undeclared shortcut" << AId;
}




// ====== Instantiation

Shortcuts::Shortcuts() :
	d(new ShortcutsPrivate)
{
	connect(Options::instance(), SIGNAL(optionsOpened()), SLOT(onOptionsOpened()));
	connect(Options::instance(), SIGNAL(optionsClosed()), SLOT(onOptionsClosed()));
}

Shortcuts::~Shortcuts()
{
}

Shortcuts *Shortcuts::instance()
{
	static Shortcuts *instance = NULL;
	if (instance == NULL)
		instance = new Shortcuts();
	return instance;
}

// ====== Public API

void Shortcuts::declareGroup(const QString &AId, const QString &ADescription)
{
	instance()->d->declareGroup(AId, ADescription);
}

void Shortcuts::declare(const QString &AId, const QString &ADescription, const QString &ALiteralKey,
						QKeySequence::StandardKey AStandardKey, Qt::ShortcutContext AContext)
{
	instance()->d->declare(AId, ADescription, ALiteralKey, AStandardKey, AContext);
}

void Shortcuts::declare(const QString &AId, const QString &ADescription, Qt::ShortcutContext AContext)
{
	instance()->d->declare(AId, ADescription, QString::null, QKeySequence::UnknownKey, AContext);
}

void Shortcuts::bind(const QString &AId, QObject *AObject)
{
	instance()->d->bind(AId, AObject);
}

void Shortcuts::update(const QString &AId, const QKeySequence &ANewKey)
{
	instance()->d->update(AId, ANewKey);
}

const Shortcuts::ShortcutMap Shortcuts::shortcutMap()
{
	return instance()->d->FShortcuts;
}

const Shortcuts::GroupMap Shortcuts::groupMap()
{
	return instance()->d->FGroups;
}

// ====== Options backend usage

void Shortcuts::onOptionsOpened()
{
	// Load settings
	//qDebug() << this << "Options opened";

	OptionsNode options = Options::node("shortcuts");
	for (ShortcutMap::iterator shortcut = d->FShortcuts.begin();
		shortcut != d->FShortcuts.end(); shortcut++)
	{
		if (options.hasValue(shortcut.key()))
		{
			shortcut->FUserSetKeySequence = options.value(shortcut.key()).toString();
			qDebug() << this << "Loaded from settings:" << shortcut.key();
		}
		else
			shortcut->FUserSetKeySequence = QKeySequence();
	}
}

void Shortcuts::onOptionsClosed()
{
	// Save settings
	//qDebug() << this << "Options closing";

	OptionsNode options = Options::node("shortcuts");
	options.removeChilds(); // Wipe out all old settings

	for (ShortcutMap::const_iterator shortcut = d->FShortcuts.constBegin();
		shortcut != d->FShortcuts.constEnd(); shortcut++)
	{
		if (shortcut->selectKeySequence() != shortcut->FDefaultKeySequence)
			options.setValue(shortcut->FUserSetKeySequence.toString(), shortcut.key());
	}
}
