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
#include "enchantchecker.h"

#include <QDir>
#include <QCoreApplication>

QList<QString> dicts;
static void enumerate_dicts (const char * const lang_tag,
							 const char * const provider_name,
							 const char * const provider_desc,
							 const char * const provider_file,
							 void * user_data)
{
	Q_UNUSED(provider_name); Q_UNUSED(provider_desc); Q_UNUSED(provider_file); Q_UNUSED(user_data);
	if(!dicts.contains(lang_tag))
		dicts+=lang_tag;
}

EnchantChecker::EnchantChecker()
{
	FDict = NULL;
	FBroker = enchant_broker_init();
}

EnchantChecker::~EnchantChecker()
{
	if (FDict)
		enchant_broker_free_dict(FBroker,FDict);
	enchant_broker_free(FBroker);
}

bool EnchantChecker::available() const
{
	return FDict != NULL;
}

bool EnchantChecker::writable() const
{
	return true;
}

QString EnchantChecker::actuallLang()
{
	return FActualLang;
}

void EnchantChecker::setLang(const QString &ALang)
{
	if (FActualLang != ALang)
	{
		FActualLang = ALang;
		if (FDict)
			enchant_broker_free_dict(FBroker,FDict);
		FDict = enchant_broker_request_dict(FBroker,ALang.toUtf8().constData());
		loadPersonalDict();
	}
}

QList< QString > EnchantChecker::dictionaries()
{
	dicts.clear();
	enchant_broker_list_dicts(FBroker,enumerate_dicts,NULL);
	return dicts;
}

void EnchantChecker::setCustomDictPath(const QString &APath)
{
	Q_UNUSED(APath);
}

void EnchantChecker::setPersonalDictPath(const QString &APath)
{
	FPersonalDictPath = APath;
}

bool EnchantChecker::isCorrect(const QString &AWord)
{
	if(available())
		return enchant_dict_check(FDict,AWord.toUtf8().constData(),AWord.toUtf8().length())<=0;
	return true;
}

bool EnchantChecker::canAdd(const QString &AWord)
{
	return writable() && !AWord.trimmed().isEmpty();
}

bool EnchantChecker::add(const QString &AWord)
{
	if (available() && canAdd(AWord))
	{
		QString trimmed_word = AWord.trimmed();
		enchant_dict_add_to_personal(FDict,trimmed_word.toUtf8().constData(),trimmed_word.toUtf8().length());
		savePersonalDict(trimmed_word);
		return true;
	}
	return false;
}

QList<QString> EnchantChecker::suggestions(const QString &AWord)
{
	QList<QString> words;
	if (available())
	{
		size_t suggestsCount = 0;
		char **suggests = enchant_dict_suggest(FDict,AWord.toUtf8().constData(),AWord.toUtf8().length(),&suggestsCount);
		if (suggests && suggestsCount>0)
		{
			for (size_t i=0; i<suggestsCount; i++)
				words.append(QString::fromUtf8(suggests[i]));
			enchant_dict_free_string_list(FDict,suggests);
		}
	}
	return words;
}

void EnchantChecker::loadPersonalDict()
{
	if (available() && !FPersonalDictPath.isEmpty())
	{
		QDir dictDir(FPersonalDictPath);
		QFile file(dictDir.absoluteFilePath(PERSONAL_DICT_FILENAME));
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			while (!file.atEnd())
			{
				QString word = QString::fromUtf8(file.readLine()).trimmed();
				if (canAdd(word))
					enchant_dict_add_to_personal(FDict,word.toUtf8().constData(),word.toUtf8().length());
			}
			file.close();
		}
	}
}

void EnchantChecker::savePersonalDict(const QString &AWord)
{
	if (!FPersonalDictPath.isEmpty() && !AWord.isEmpty())
	{
		QDir dictDir(FPersonalDictPath);
		QFile file(dictDir.absoluteFilePath(PERSONAL_DICT_FILENAME));
		if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		{
			file.write(AWord.toUtf8());
			file.write("\n");
			file.close();
		}
	}
}
