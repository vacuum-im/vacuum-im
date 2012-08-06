#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>

#include "spellhighlighter.h"

#define SPELLCHECKER_UUID "{0dc5fbd9-2dd4-4720-9c95-8c3393a577a5}"

class QAction;
class QMenu;
class QPoint;
class SpellHighlighter;

struct SHPair
{
	QObject *attachedTo;
	SpellHighlighter *spellHighlighter;
};

class SpellChecker : 
	public QObject,
	public IPlugin
{
	Q_OBJECT
	Q_INTERFACES(IPlugin)
public:
	SpellChecker();
	~SpellChecker();
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SPELLCHECKER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	static QString dictPath();
	//static QString homePath();
protected:
	void appendHL(QTextDocument *ADocument, IMultiUserChat *AMultiUserChat);
	SpellHighlighter *getSpellByDocument(QObject *ADocument, int *index);
	SpellHighlighter *getSpellByDocumentAndRemove(QObject *ADocument);
protected slots:
	void onEditWidgetCreated(IEditWidget *AWidget);
	void onSpellDocumentDestroyed(QObject *ADocument);
	void showContextMenu(const QPoint &pt);
	void repairWord();
	void setDict();
	void addWordToDict();
private slots:
	void resetMenu();
	void start();
	void suggestions(const QString &word, const QStringList &words);
private:
	void suggestionsMenu(const QString &word, QMenu *parent);
private:
	IPluginManager *FPluginManager;
	IMessageWidgets *FMessageWidgets;
private:
	QTextEdit* FCurrentTextEdit;
	int FCurrentCursorPosition;
	QList<SHPair> FHighlighWidgets;
	QMenu *FDictMenu;
	QStringList enabledDicts;
private:
	QMenu *suggestMenu(const QString &word);
	QMenu *dictMenu();
};

#endif // SPELLCHECKER_H
