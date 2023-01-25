#ifndef NUSPELLCHECKER_H
#define NUSPELLCHECKER_H

#include <QList>
#include <QString>
#include "spellbackend.h"

#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

class NuspellChecker : public SpellBackend
{
public:
	NuspellChecker();
	~NuspellChecker();
	virtual bool available() const;
	virtual QString actuallLang();
	virtual void setLang(const QString &ALang);
	virtual QList<QString> dictionaries();
	virtual bool isCorrect(const QString &AWord);
	virtual QList<QString> suggestions(const QString &AWord);
private:
	void loadNuspell(const QString &ALang);
	void loadPersonalDict();
	void savePersonalDict(const QString &AWord);
private:
	QString FActualLang;
	QString FPersonalDictPath;
	QList<QString> FDictsPaths;
	QList<QString> FDicts;

	nuspell::Dictionary FNuspell;
	std::vector<std::pair<std::string, std::string>> dict_list;
};

#endif
