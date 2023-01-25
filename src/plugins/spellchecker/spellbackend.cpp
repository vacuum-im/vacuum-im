#include "spellbackend.h"

bool SpellBackend::available() const
{
	return false;
}

bool SpellBackend::writable() const
{
	return false;
}

QString SpellBackend::actuallLang()
{
	return QString();
}

void SpellBackend::setLang(const QString &ALang)
{
	Q_UNUSED(ALang);
}

QList< QString > SpellBackend::dictionaries()
{
	return QList<QString>();
}

void SpellBackend::setCustomDictPath(const QString &APath)
{
	Q_UNUSED(APath);
}

void SpellBackend::setPersonalDictPath(const QString &APath)
{
	Q_UNUSED(APath);
}

bool SpellBackend::isCorrect(const QString &AWord)
{
	Q_UNUSED(AWord);
	return true;
}

bool SpellBackend::canAdd(const QString &AWord)
{
	Q_UNUSED(AWord);
	return writable();
}

bool SpellBackend::add(const QString &AWord)
{
	Q_UNUSED(AWord);
	return false;
}

QList<QString> SpellBackend::suggestions(const QString &AWord)
{
	Q_UNUSED(AWord);
	return QList<QString>();
}
