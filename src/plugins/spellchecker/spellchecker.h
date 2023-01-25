#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <QHash>

#include <interfaces/ispellchecker.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>
#include "spellhighlighter.h"
#include "spellbackend.h"

class SpellChecker : 
	public QObject,
	public IPlugin,
	public ISpellChecker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISpellChecker);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.SpellChecker");
public:
	SpellChecker();
	~SpellChecker();
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SPELLCHECKER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
public:
	virtual bool isSpellEnabled() const;
	virtual void setSpellEnabled(bool AEnabled);
	virtual bool isSpellAvailable() const;
	virtual QList<QString> availDictionaries() const;
	virtual QString currentDictionary() const;
	virtual void setCurrentDictionary(const QString &ADict);
	virtual bool isCorrectWord(const QString &AWord) const;
	virtual QList<QString> wordSuggestions(const QString &AWord) const;
	virtual bool canAddWordToPersonalDict(const QString &AWord) const;
	virtual void addWordToPersonalDict(const QString &AWord);
signals:
	void spellEnableChanged(bool AEnabled);
	void currentDictionaryChanged(const QString &ADict);
	void wordAddedToPersonalDict(const QString &AWord);
protected:
	void rehightlightAll();
	void loadDictionary(const QString &ADict);
	QString dictionaryName(const QString &ADict) const;
protected slots:
	void onChangeSpellEnable();
	void onChangeDictionary();
	void onRepairWordUnderCursor();
	void onAddUnknownWordToDictionary();
	void onEditWidgetCreated(IMessageEditWidget *AWidget);
	void onEditWidgetContextMenuRequested(const QPoint &APosition, Menu *AMenu);
	void onTextEditDestroyed(QObject *AObject);
protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPluginManager *FPluginManager;
	IMessageWidgets *FMessageWidgets;
private:
	SpellBackend *FSpellBackend;
	QTextEdit *FCurrentTextEdit;
	int FCurrentCursorPosition;
	QHash<QString, bool> *FCachedWords;
	QHash<QString, QList<QString>> *FCachedSuggs;
	QMap<QObject *, SpellHighlighter *> FSpellHighlighters;
	QString FCurrectDict;
};

#endif // SPELLCHECKER_H
