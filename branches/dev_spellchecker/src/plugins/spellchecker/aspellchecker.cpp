/*
 * aspellchecker.cpp
 *
 * Copyright (C) 2006  Remko Troncon
 * Thanks to Ephraim.
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
#include <QLocale>
#include <QCoreApplication>
#include <QtDebug>

#include <aspell.h>
#include "aspellchecker.h"

ASpellChecker::ASpellChecker()
{
	FConfig = NULL;
	FSpeller = NULL;
	FConfig = new_aspell_config();
	lang = QLocale().name().toUtf8().constData();
	aspell_config_replace(FConfig, "encoding", "utf-8");
	aspell_config_replace(FConfig, "lang", lang.toUtf8().constData());
#ifdef Q_WS_WIN
	aspell_config_replace(FConfig, "conf-dir", QString("%1/aspell").arg(QCoreApplication::applicationDirPath()).toUtf8().constData());
	aspell_config_replace(FConfig, "data-dir", QString("%1/aspell/data").arg(QCoreApplication::applicationDirPath()).toUtf8().constData());
	aspell_config_replace(FConfig, "dict-dir", QString("%1/aspell/dict").arg(QCoreApplication::applicationDirPath()).toUtf8().constData());
#endif

	AspellCanHaveError* ret = new_aspell_speller(FConfig);
	if (aspell_error_number(ret) == 0) 
	{
		FSpeller = to_aspell_speller(ret);
	}
	else 
	{
		qWarning() << QString("Aspell error: %1").arg(aspell_error_message(ret));
		delete_aspell_can_have_error(ret);
	}
}

ASpellChecker::~ASpellChecker()
{
	if(FConfig) 
	{
		delete_aspell_config(FConfig);
		FConfig = NULL;
	}

	if(FSpeller) 
	{
		delete_aspell_speller(FSpeller);
		FSpeller = NULL;
	}
}

bool ASpellChecker::isCorrect(const QString &AWord)
{
	if(FSpeller) 
	{
		int correct = aspell_speller_check(FSpeller,AWord.toUtf8().constData(), -1);
		return (correct != 0);
	}
	return true;
}

QList<QString> ASpellChecker::suggestions(const QString &AWord)
{
	QList<QString> words;
	if (FSpeller) 
	{
		const AspellWordList* list = aspell_speller_suggest(FSpeller, AWord.toUtf8().constData(), -1); 
		AspellStringEnumeration* elements = aspell_word_list_elements(list);
	
		const char *c_word;
		while ((c_word = aspell_string_enumeration_next(elements)) != NULL) 
			words += QString::fromUtf8(c_word);

		delete_aspell_string_enumeration(elements);
	}
	return words;
}

bool ASpellChecker::add(const QString &AWord)
{
	bool result = false;
	if (FConfig && FSpeller) 
	{
		QString trimmed_word = AWord.trimmed();
		if(!trimmed_word.isEmpty()) 
		{
			aspell_speller_add_to_personal(FSpeller, trimmed_word.toUtf8(), trimmed_word.toUtf8().length());
			aspell_speller_save_all_word_lists(FSpeller);
			result = true;
		}
	}
	return result;
}

bool ASpellChecker::available() const
{
	return (FSpeller != NULL);
}

bool ASpellChecker::writable() const
{
	return false;
}

QList< QString > ASpellChecker::dictionaries()
{
  AspellDictInfoEnumeration * dels;
  const AspellDictInfo * entry;
	QList<QString> dict;

  dels = aspell_dict_info_list_elements(get_aspell_dict_info_list(FConfig));

  while ( (entry = aspell_dict_info_enumeration_next(dels)) != 0) {
		if(!dict.contains(QString::fromUtf8(entry->code)))
		 dict += QString::fromUtf8(entry->code);
	}

  delete_aspell_dict_info_enumeration(dels);
	
	return dict;
}

void ASpellChecker::setLang(const QString &AWord)
{
	lang = AWord;
	aspell_config_replace(FConfig, "lang", AWord.toUtf8());
	AspellCanHaveError* ret = new_aspell_speller(FConfig);
	if (aspell_error_number(ret) == 0) 
	{
		if(FSpeller) 
			delete_aspell_speller(FSpeller);
		
		FSpeller = to_aspell_speller(ret);
	}
	else 
	{
		qWarning() << QString("Aspell error: %1").arg(aspell_error_message(ret));
	}
}

QString ASpellChecker::actuallLang()
{
	return lang;
}
