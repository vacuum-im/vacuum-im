/*
 * spellchecker.cpp
 *
 * Copyright (C) 2006  Remko Troncon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "spellbackend.h"

#include <QCoreApplication>

#if defined(HAVE_MACSPELL)
#	include "macspellchecker.h"
#elif defined(HAVE_ENCHANT)
#	include "enchantchecker.h"
#elif defined(HAVE_ASPELL)
#	include "aspellchecker.h"
#elif defined(HAVE_HUNSPELL)
#	include "hunspellchecker.h"
#endif

SpellBackend *SpellBackend::FInstance = NULL;

SpellBackend::SpellBackend() : QObject(QCoreApplication::instance())
{

}

SpellBackend::~SpellBackend()
{

}

SpellBackend *SpellBackend::instance()
{
	if (!FInstance)
	{
#if defined(HAVE_MACSPELL)
		FInstance = new MacSpellChecker();
#elif defined (HAVE_ENCHANT)
		FInstance = new EnchantChecker();
#elif defined(HAVE_ASPELL)
		FInstance = new ASpellChecker();
#elif defined(HAVE_HUNSPELL)
		FInstance = new HunspellChecker();
#else
		FInstance = new SpellBackend();
#endif
	}
	return FInstance;
}

void SpellBackend::destroyInstance()
{
	delete FInstance;
	FInstance = NULL;
}

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
	return QString::null;
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
