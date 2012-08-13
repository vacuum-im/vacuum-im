#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>
#include <utils/options.h>
#include "spellhighlighter.h"

#define SPELLCHECKER_UUID "{0dc5fbd9-2dd4-4720-9c95-8c3393a577a5}"

class SpellChecker : 
	public QObject,
	public IPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin);
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
protected slots:
	void onChangeSpellEnable();
	void onChangeDictionary();
	void onRepairWordUnderCursor();
	void onAddUnknownWordToDictionary();
	void onEditWidgetCreated(IEditWidget *AWidget);
	void onEditWidgetContextMenuRequested(const QPoint &APosition, Menu *AMenu);
	void onTextEditDestroyed(QObject *AObject);
protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPluginManager *FPluginManager;
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FDictMenu;
	QTextEdit *FCurrentTextEdit;
	int FCurrentCursorPosition;
	QMap<QObject *, SpellHighlighter *> FSpellHighlighters;
};

#endif // SPELLCHECKER_H
