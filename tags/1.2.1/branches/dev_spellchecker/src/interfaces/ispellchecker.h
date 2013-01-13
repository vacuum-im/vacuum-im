#ifndef ISPELLCHECKER_H
#define ISPELLCHECKER_H

#include <QObject>
#include <QString>

#define SPELLCHECKER_UUID "{0dc5fbd9-2dd4-4720-9c95-8c3393a577a5}"

class ISpellChecker
{
public:
	virtual QObject *instance() = 0;
	virtual bool isSpellEnabled() const =0;
	virtual void setSpellEnabled(bool AEnabled) =0;
	virtual bool isSpellAvailable() const =0;
	virtual QList<QString> availDictionaries() const =0;
	virtual QString currentDictionary() const =0;
	virtual void setCurrentDictionary(const QString &ADict) =0;
	virtual bool isCorrectWord(const QString &AWord) const =0;
	virtual QList<QString> wordSuggestions(const QString &AWord) const =0;
	virtual bool canAddWordToPersonalDict(const QString &AWord) const =0;
	virtual void addWordToPersonalDict(const QString &AWord) =0;
protected:
	virtual void spellEnableChanged(bool AEnabled) =0;
	virtual void currentDictionaryChanged(const QString &ADict) =0;
	virtual void wordAddedToPersonalDict(const QString &AWord) =0;
};

Q_DECLARE_INTERFACE(ISpellChecker,"Vacuum.Plugin.ISpellChecker/1.0")

#endif // ISPELLCHECKER_H
