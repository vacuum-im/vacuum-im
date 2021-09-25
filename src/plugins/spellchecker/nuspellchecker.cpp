#include "nuspellchecker.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QCoreApplication>
#include <utils/logger.h>
#include "spellchecker.h"

NuspellChecker::NuspellChecker()
{
	nuspell::search_default_dirs_for_dicts(dict_list);
	foreach(auto dict, dict_list)
	{
		FDicts.append(QString::fromStdString(dict.first));
		FDicts.removeDuplicates();
	}
}

NuspellChecker::~NuspellChecker()
{
}

bool NuspellChecker::available() const
{
	return FDicts.isEmpty() ? false : true;
}

QString NuspellChecker::actuallLang()
{
	return FActualLang;
}

void NuspellChecker::setLang(const QString &ALang)
{
	if (FActualLang != ALang)
	{
		FActualLang = ALang;
		loadNuspell(FActualLang);
	}
}

QList<QString> NuspellChecker::dictionaries()
{
	return FDicts;
}

bool NuspellChecker::isCorrect(const QString &AWord)
{
	if(available())
	{
		return FNuspell.spell(AWord.toStdString());
	}
	return true;
}

QList<QString> NuspellChecker::suggestions(const QString &AWord)
{
	QList<QString> words;
	if(available())
	{
		std::vector<std::string> suggs;
		FNuspell.suggest(AWord.toStdString(), suggs);
		foreach(std::string sug, suggs)
			words.append(QString::fromStdString(sug));
	}
	return words;
}

void NuspellChecker::loadNuspell(const QString &ALang)
{
	auto dict_name_and_path = nuspell::find_dictionary(dict_list, ALang.toStdString());
	if (dict_name_and_path == end(dict_list))
		return;
	auto &dict_path = dict_name_and_path->second;

	FNuspell = nuspell::Dictionary::load_from_path(dict_path);
}
