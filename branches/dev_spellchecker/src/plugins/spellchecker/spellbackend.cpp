#include <QCoreApplication>

#include "hunspellchecker.h"
#include "spellbackend.h"

#if defined(HAVE_MACSPELL)
#	include "macspellchecker.h"
#elif defined(HAVE_ENCHANT)
#	include "enchantchecker.h"
#elif defined(HAVE_ASPELL)
#	include "aspellchecker.h"
#elif defined(HAVE_HUNSPELL)
#	include "hunspellchecker.h"
#endif

SpellBackend* SpellBackend::FSelf = 0;

SpellBackend* SpellBackend::instance()
{
#ifdef HAVE_ENCHANT
		FSelf = new EnchantChecker();
#elif defined(HAVE_ASPELL)
		FSelf = new ASpellChecker();
#elif defined(HAVE_HUNSPELL)
		FSelf = new HunspellChecker();
#else
		FSelf = new SpellBackend();
#endif
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
