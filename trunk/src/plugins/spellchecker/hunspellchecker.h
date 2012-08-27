#ifndef HUNSPELLCHECKER_H
#define HUNSPELLCHECKER_H

#include <QList>
#include <QString>
#include <QTextCodec>
#include "spellbackend.h"

class Hunspell;

class HunspellChecker :
	public SpellBackend
{
public:
	HunspellChecker();
	~HunspellChecker();
	virtual bool available() const;
	virtual bool writable() const;
	virtual QString actuallLang();
	virtual void setLang(const QString &ALang);
	virtual QList<QString> dictionaries();
	virtual void setCustomDictPath(const QString &APath);
	virtual void setPersonalDictPath(const QString &APath);
	virtual bool isCorrect(const QString &AWord);
	virtual bool canAdd(const QString &AWord);
	virtual bool add(const QString &AWord);
	virtual QList<QString> suggestions(const QString &AWord);
private:
	void loadHunspell(const QString &ALang);
	void loadPersonalDict();
	void savePersonalDict(const QString &AWord);
private:
	Hunspell *FHunSpell;
	static const QString FDictsPath;
	QString FActualLang;
	QTextCodec *FDictCodec;
	QString FPersonalDictPath;
};

#endif
