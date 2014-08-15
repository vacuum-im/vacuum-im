#include "hunspellchecker.h"

#ifdef USE_SYSTEM_HUNSPELL
#	include <hunspell/hunspell.hxx>
#else
#	include <thirdparty/hunspell/hunspell.hxx>
#endif

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QCoreApplication>
#include <utils/logger.h>
#include "spellchecker.h"

HunspellChecker::HunspellChecker() : FHunSpell(NULL), FDictCodec(NULL)
{
#if defined (Q_OS_WIN)
	FDictsPaths.append(QString("%1/hunspell").arg(QCoreApplication::applicationDirPath()));
#elif defined (Q_OS_LINUX)
	FDictsPaths.append("/usr/share/hunspell");
	FDictsPaths.append("/usr/share/myspell");
#elif defined (Q_OS_MAC)
	FDictsPaths.append(QString("%1/Library/Spelling").arg(QDir::homePath()));
#endif
}

HunspellChecker::~HunspellChecker()
{
	delete FHunSpell;
}

bool HunspellChecker::available() const
{
	return FHunSpell!=NULL;
}

bool HunspellChecker::writable() const
{
	return !FPersonalDictPath.isEmpty();
}

QString HunspellChecker::actuallLang()
{
	return FActualLang;
}

void HunspellChecker::setLang(const QString &ALang)
{
	if (FActualLang != ALang)
	{
		FActualLang = ALang;
		loadHunspell(FActualLang);
	}
}
QList<QString> HunspellChecker::dictionaries()
{
	QList<QString> availDicts;
	foreach(const QString &dictsPath, FDictsPaths)
	{
		QDir dir(dictsPath);
		foreach(QString dictFile, dir.entryList(QStringList("*.dic"),QDir::Files|QDir::Readable, QDir::Name|QDir::IgnoreCase))
		{
			if (dictFile.startsWith("hyph_"))
				continue;
			dictFile = dictFile.mid(0,dictFile.length()-4);
			if (!availDicts.contains(dictFile))
				availDicts.append(dictFile);
		}
	}
	return availDicts;
}

void HunspellChecker::setCustomDictPath(const QString &APath)
{
	Q_UNUSED(APath);
}

void HunspellChecker::setPersonalDictPath(const QString &APath)
{
	FPersonalDictPath = APath;
}

bool HunspellChecker::isCorrect(const QString &AWord)
{
	if(available())
	{
		QByteArray encWord = FDictCodec!=NULL ? FDictCodec->fromUnicode(AWord) : AWord.toUtf8();
		return FHunSpell->spell(encWord.constData());
	}
	return true;
}

bool HunspellChecker::canAdd(const QString &AWord)
{
	QString trimmedWord = AWord.trimmed();
	if (writable() && !trimmedWord.isEmpty())
		return FDictCodec!=NULL ? FDictCodec->canEncode(trimmedWord) : true;
	return false;
}

bool HunspellChecker::add(const QString &AWord)
{
	if (available() && canAdd(AWord))
	{
		QString trimmedWord = AWord.trimmed();
		QByteArray encWord = FDictCodec!=NULL ? FDictCodec->fromUnicode(trimmedWord) : trimmedWord.toUtf8();
		FHunSpell->add(encWord.constData());
		savePersonalDict(trimmedWord);
		return true;
	}
	return false;
}

QList<QString> HunspellChecker::suggestions(const QString &AWord)
{
	QList<QString> words;
	if(available())
	{
		char **sugglist;
		QByteArray encWord = FDictCodec ? FDictCodec->fromUnicode(AWord) : AWord.toUtf8();
		int count = FHunSpell->suggest(&sugglist, encWord.data());
		for(int i = 0; i < count; ++i)
			words.append(FDictCodec ? FDictCodec->toUnicode(sugglist[i]) : QString::fromUtf8(sugglist[i]));
		FHunSpell->free_list(&sugglist, count);
	}
	return words;
}

void HunspellChecker::loadHunspell(const QString &ALang)
{
	delete FHunSpell;
	FHunSpell = NULL;

	foreach(const QString &dictsPath, FDictsPaths)
	{
		QString dictFile = QString("%1/%2.dic").arg(dictsPath).arg(ALang);
		if (QFileInfo(dictFile).exists())
		{
			QString rulesFile = QString("%1/%2.aff").arg(dictsPath).arg(ALang);
			FHunSpell = new Hunspell(rulesFile.toLocal8Bit().constData(), dictFile.toLocal8Bit().constData());
			FDictCodec = QTextCodec::codecForName(FHunSpell->get_dic_encoding());
			loadPersonalDict();
			break;
		}
	}
}

void HunspellChecker::loadPersonalDict()
{
	if (available() && !FPersonalDictPath.isEmpty())
	{
		QDir dictDir(FPersonalDictPath);
		QFile file(dictDir.absoluteFilePath(PERSONAL_DICT_FILENAME));
		if (file.open(QIODevice::ReadOnly|QIODevice::Text))
		{
			while (!file.atEnd())
			{
				QString word = QString::fromUtf8(file.readLine()).trimmed();
				if (canAdd(word))
				{
					QByteArray encWord= FDictCodec!=NULL ? FDictCodec->fromUnicode(word) : word.toUtf8();
					FHunSpell->add(encWord.constData());
				}
			}
		}
		else if (file.exists())
		{
			REPORT_ERROR(QString("Failed to load personal dictionary from file: %1").arg(file.errorString()));
		}
	}
}

void HunspellChecker::savePersonalDict(const QString &AWord)
{
	if (!FPersonalDictPath.isEmpty() && !AWord.isEmpty())
	{
		QDir dictDir(FPersonalDictPath);
		QFile file(dictDir.absoluteFilePath(PERSONAL_DICT_FILENAME));
		if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		{
			file.write(AWord.toUtf8());
			file.write("\n");
			file.flush();
		}
		else
		{
			REPORT_ERROR(QString("Failed to save personal dictionary to file: %1").arg(file.errorString()));
		}
	}
}
