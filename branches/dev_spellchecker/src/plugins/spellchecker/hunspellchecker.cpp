#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QList>
#include <QLocale>
#include <QString>
#include <QThreadPool>
#include <QTextCodec>
#include <QDebug>

#include "hunspellchecker.h"

HunspellChecker::HunspellChecker(QObject *parent) : SpellBackend(parent)
{
	FPool = new QThreadPool(this);
	FPool->setMaxThreadCount(1);

	dictPath = SpellChecker::dictPath();
}

HunspellChecker::~HunspellChecker()
{
	clear();
}

bool HunspellChecker::add(const QString &AWord)
{
	bool result = false;
//	if (FHunSpell)
//	{
//		QFile file(personalDict);
//		file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
//		file.write(AWord.toUtf8());
//		file.write("\n");
//		file.close();
//		QByteArray encodedString;
//		encodedString = codec->fromUnicode(AWord);
//		FHunSpell->add(encodedString.data());
//		result = true;
//	}
	return result;
}


bool HunspellChecker::available() const
{
	return true;
}

bool HunspellChecker::isCorrect(const QString &AWord) const
{
	int sum = 0;
	if (!FMutex.tryLock())
		return false;
	foreach (Hunspell *dic, FHunspellList)
	{
		QByteArray encoded = QTextCodec::codecForName(dic->get_dic_encoding())->fromUnicode(AWord);
		sum += dic->spell(encoded.constData());
	}
	FMutex.unlock();
	return sum > 0;
}

QStringList HunspellChecker::dictionaries() const
{
	QStringList dict;
	QDir dir(dictPath);
	if (dir.exists())
	{
		QStringList lstDic = dir.entryList(QStringList("*.dic"), QDir::Files);
		foreach(QString tmp, lstDic)
		{
			if (tmp.startsWith("hyph_"))
				continue;
			if (tmp.startsWith("th_"))
				continue;
			if (tmp.endsWith(".dic"))
				tmp = tmp.mid(0, tmp.length() - 4);
			dict << tmp;
		}
	}
	return dict;
}

QStringList HunspellChecker::suggestions(const QString &AWord) const
{
	QStringList words;

	if (!FMutex.tryLock())
		return words;

	foreach (Hunspell *dic, FHunspellList)
	{
		char **sugglist = NULL;
		QTextCodec *codec = QTextCodec::codecForName(dic->get_dic_encoding());
		QByteArray encoded = codec->fromUnicode(AWord);
		int count = dic->suggest(&sugglist, encoded.constData());
		for (int i = 0; i < count; ++i)
			words << codec->toUnicode(sugglist[i]);
		dic->free_list(&sugglist, count);
	}

	FMutex.unlock();
	return words;
}

void HunspellChecker::queuedSuggestions(const QString &AWord) const
	{
		HunspellSuggestions *task = new HunspellSuggestions(this, AWord);
		connect(task, SIGNAL(ready(QString,QStringList)), SIGNAL(suggestionsReady(QString,QStringList)));
		FPool->start(task);
	}

void HunspellChecker::setLangs(const QStringList &ADicts)
{
	QStringList files;
	foreach (const QString &name, ADicts)
	{
		files.append(dictPath + QDir::separator() + name);
	}
	FPool->start(new HunspellLoader(this, files));
}

void HunspellChecker::clear()
{
	FMutex.lock();
	qDeleteAll(FHunspellList);
	FHunspellList.clear();
	FMutex.unlock();
}

//void HunspellChecker::loadHunspell(const QString &ALang)
//{
//	QString dic = QString("%1/%2.dic").arg(dictPath).arg(ALang);
//	if (QFileInfo(dic).exists())
//	{
//		FHunSpell = new Hunspell(QString("%1/%2.aff").arg(dictPath).arg(ALang).toUtf8().constData(), dic.toUtf8().constData());
//		codec = QTextCodec::codecForName(FHunSpell->get_dic_encoding());
//		personalDict = QString("%1/personal.dictionary.txt").arg(SpellChecker::homePath);
//		QFile file(personalDict);
//		file.open(QIODevice::ReadOnly | QIODevice::Text);
//		QTextStream out(&file);
//		while (!out.atEnd())
//		{
//			QByteArray encodedString;
//			encodedString = codec->fromUnicode(out.readLine());
//			FHunSpell->add(encodedString.constData());
//		}
//		file.close();
//	}
//}

void HunspellChecker::load(const QStringList &ADicts)
{
	if (ADicts.isEmpty())
		return;
	FMutex.lock();
	foreach (const QString &name, ADicts)
	{
		if (QFile::exists(name + ".dic"))
		{
			Hunspell *dic = new Hunspell(QString(name + ".aff").toUtf8().constData(), QString(name + ".dic").toUtf8().constData());
			if (QTextCodec::codecForName(dic->get_dic_encoding()))
				FHunspellList.append(dic);
			else
				delete dic;
		}
	}
	FMutex.unlock();
}

HunspellLoader::HunspellLoader(HunspellChecker *hunspell, const QStringList &dicts)
	: QRunnable(), FHunspell(hunspell), FDictsList(dicts) {}

void HunspellLoader::run()
{
	FHunspell->clear();
	qDebug() << FDictsList;
	FHunspell->load(FDictsList);
}

HunspellSuggestions::HunspellSuggestions(const HunspellChecker *hunspell, const QString &AWord)
	: QObject(), QRunnable(), FHunspell(hunspell), FWord(AWord) {}

void HunspellSuggestions::run()
{
	QStringList words = FHunspell->suggestions(FWord);
	if (!words.isEmpty())
		emit ready(FWord, words);
}

