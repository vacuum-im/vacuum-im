#include <QCoreApplication>

#include "hunspellchecker.h"
#include "spellbackend.h"

SpellBackend* SpellBackend::FSelf = 0;

SpellBackend* SpellBackend::instance()
{
	if (!FSelf)
		FSelf = new HunspellChecker(QCoreApplication::instance());
	return FSelf;
}

SpellBackend::SpellBackend(QObject *parent) : QObject(parent) {}

bool SpellBackend::add(const QString &AWord)
{
	Q_UNUSED(AWord);
	return false;
}

bool SpellBackend::available() const
{
	return false;
}

bool SpellBackend::isCorrect(const QString &AWord) const
{
	Q_UNUSED(AWord);
	return true;
}

bool SpellBackend::writable() const
{
	return false;
}

QStringList SpellBackend::dictionaries() const
{
	return QStringList();
}

QStringList SpellBackend::suggestions(const QString &AWord) const
{
	Q_UNUSED(AWord);
	return QStringList();
}

void SpellBackend::queuedSuggestions(const QString &AWord) const
{
	Q_UNUSED(AWord);
}

QString SpellBackend::actuallLang() const
{
	return QString();
}

void SpellBackend::setLangs(const QStringList &ADicts)
{
	Q_UNUSED(ADicts)
}
