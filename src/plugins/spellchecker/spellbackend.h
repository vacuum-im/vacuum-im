#ifndef SPELLBACKEND_H
#define SPELLBACKEND_H

#include <QList>
#include <QString>
#include <QObject>

#define PERSONAL_DICT_FILENAME  "personal.dic"

class SpellBackend : public QObject
{
public:
	SpellBackend() = default;
	virtual ~SpellBackend() = default;

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
};

#endif // SPELLBACKEND_H
