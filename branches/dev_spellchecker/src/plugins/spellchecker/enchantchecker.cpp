/*
 * enchantchecker.cpp
 *
 * Copyright (C) 2009  Caolán McNamara
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

#include <QDir>
#include <QCoreApplication>
#include <QtDebug>

#include "enchant++.h"
#include "enchantchecker.h"

QList<QString> dict;
static void enumerate_dicts (const char * const lang_tag,
		 const char * const provider_name,
		 const char * const provider_desc,
		 const char * const provider_file,
		 void * user_data)
{
	if(!dict.contains(lang_tag))
		dict+=lang_tag;
}

EnchantChecker::EnchantChecker() : FSpeller(NULL)
{
	if (enchant::Broker *instance = enchant::Broker::instance())
	{
		QLocale loc;
		lang="en_US";
		dict.clear();
		instance->list_dicts(enumerate_dicts);
		if (instance->dict_exists(loc.name().toStdString()))
			lang = loc.name().toStdString();
		try {
			FSpeller = enchant::Broker::instance()->request_dict(lang);
		} catch (enchant::Exception &e) {
			qWarning() << QString("Enchant error: %1").arg(e.what());
		}
	}
}

EnchantChecker::~EnchantChecker()
{
	delete FSpeller;
	FSpeller = NULL;
}

bool EnchantChecker::isCorrect(const QString &AWord)
{
	if(FSpeller) 
	{
		return FSpeller->check(AWord.toUtf8().constData());
	}
	return true;
}

QList<QString> EnchantChecker::suggestions(const QString &AWord)
{
	QList<QString> words;
	if (FSpeller) 
	{
		std::vector<std::string> out_suggestions;
		FSpeller->suggest(AWord.toUtf8().constData(), out_suggestions);
		std::vector<std::string>::iterator aE = out_suggestions.end();
		for (std::vector<std::string>::iterator aI = out_suggestions.begin(); aI != aE; ++aI)
			words += QString::fromUtf8(aI->c_str());
	}
	return words;
}

bool EnchantChecker::add(const QString &AWord)
{
	bool result = false;
	if (FSpeller) 
	{
		QString trimmed_word = AWord.trimmed();
		if(!trimmed_word.isEmpty()) 
		{
			FSpeller->add_to_pwl(trimmed_word.toUtf8().constData());
			result = true;
		}
	}
	return result;
}

bool EnchantChecker::available() const
{
	return (FSpeller != NULL);
}

bool EnchantChecker::writable() const
{
	return false;
}

QList< QString > EnchantChecker::dictionaries()
{
	return dict;
}

void EnchantChecker::setLang(const QString &AWord)
{
	if (enchant::Broker *instance = enchant::Broker::instance())
	{
		dict.clear();
		instance->list_dicts(enumerate_dicts);
		if (instance->dict_exists(AWord.toStdString()))
			lang = AWord.toStdString();
		try {
			FSpeller = enchant::Broker::instance()->request_dict(lang);
		} catch (enchant::Exception &e) {
			qWarning() << QString("Enchant error: %1").arg(e.what());
		}
	}
}

QString EnchantChecker::actuallLang()
{
	return QString::fromStdString(lang);
}
