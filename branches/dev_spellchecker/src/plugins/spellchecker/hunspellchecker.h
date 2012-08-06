#ifndef HUNSPELLCHECKER_H
#define HUNSPELLCHECKER_H

#include <QMap>
#include <QMutex>
#include <QRunnable>

#include <thirdparty/hunspell/hunspell.hxx>
#include "spellbackend.h"
#include "spellchecker.h"

class Hunspell;
class QThreadPool;

class HunspellChecker : public SpellBackend
{
public:
	HunspellChecker(QObject *parent = 0);
	~HunspellChecker();
	bool add(const QString &AWord);
	bool available() const;
	bool isCorrect(const QString &AWord) const;
	QStringList dictionaries() const;
	QStringList suggestions(const QString &AWord) const;
	void setLangs(const QStringList &ADicts);
	void queuedSuggestions(const QString &AWord) const;
public:
	void clear();
	void load(const QStringList &ADicts);
private:
	mutable QMutex FMutex;
	QList<Hunspell*> FHunspellList;
	QThreadPool *FPool;
private:
	QString dictPath;
	QString personalDict;
};

class HunspellLoader : public QRunnable
{
public:
	HunspellLoader(HunspellChecker *hunspell, const QStringList &dicts);
	void run();
private:
	HunspellChecker *FHunspell;
	QStringList FDictsList;
};

class HunspellSuggestions : public QObject, public QRunnable
{
	Q_OBJECT
public:
	HunspellSuggestions(const HunspellChecker *hunspell, const QString &AWord);
	void run();
signals:
	void ready(const QString &AWord, const QStringList &AWords);
private:
	const HunspellChecker *FHunspell;
	QString FWord;
};

#endif
