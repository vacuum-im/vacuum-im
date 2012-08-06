#ifndef SPELLBACKEND_H
#define SPELLBACKEND_H

#include <QObject>
#include <QStringList>

class SpellBackend : public QObject
{
	Q_OBJECT
public:
	static SpellBackend* instance();
	virtual bool add(const QString &AWord);
	virtual bool available() const;
	virtual bool isCorrect(const QString &AWord) const;
	virtual bool writable() const;
	virtual QString actuallLang() const;
	virtual QStringList dictionaries() const;
	virtual QStringList suggestions(const QString &AWord) const;
	virtual void queuedSuggestions(const QString &AWord) const;
	virtual void setLangs(const QStringList &ADicts);

signals:
	void suggestionsReady(const QString &AWord, const QStringList &AWords);

protected:
	SpellBackend(QObject *parent = 0);

private:
	static SpellBackend *FSelf;
};

#endif // SPELLBACKEND_H
