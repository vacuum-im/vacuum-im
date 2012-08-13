#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QApplication>

#include "spellbackend.h"
#include "spellchecker.h"

#define SPELLDICTS_DIR      "spelldicts"
#define PERSONALDICTS_DIR   "personal"

SpellChecker::SpellChecker()
{
	FMessageWidgets = NULL;

	FDictMenu = NULL;
	FCurrentTextEdit = NULL;
	FCurrentCursorPosition = 0;
}

SpellChecker::~SpellChecker()
{
	delete FDictMenu;
	SpellBackend::destroyInstance();
}

void SpellChecker::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Spell Checker");
	APluginInfo->description = tr("Highlights words that may not be spelled correctly");
	APluginInfo->version = "0.0.7";
	APluginInfo->author = "Minnahmetov V.K.";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
}

bool SpellChecker::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(editWidgetCreated(IEditWidget *)),SLOT(onEditWidgetCreated(IEditWidget *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FMessageWidgets != NULL;
}

bool SpellChecker::initObjects()
{
	FDictMenu = new Menu;
	FDictMenu->setTitle(tr("Dictionary"));

	QDir dictsDir(FPluginManager->homePath());
	if (!dictsDir.exists(SPELLDICTS_DIR))
		dictsDir.mkdir(SPELLDICTS_DIR);
	dictsDir.cd(SPELLDICTS_DIR);
	SpellBackend::instance()->setCustomDictPath(dictsDir.absolutePath());

	if (!dictsDir.exists(PERSONALDICTS_DIR))
		dictsDir.mkdir(PERSONALDICTS_DIR);
	dictsDir.cd(PERSONALDICTS_DIR);
	SpellBackend::instance()->setPersonalDictPath(dictsDir.absolutePath());

	return true;
}

bool SpellChecker::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SPELL_ENABLED,true);
	Options::setDefaultValue(OPV_MESSAGES_SPELL_LANG,QLocale().name());
	return true;
}

bool SpellChecker::startPlugin()
{
	foreach(QString dict, SpellBackend::instance()->dictionaries())
	{
		Action *action = new Action(FDictMenu);
		action->setText(dict);
		action->setProperty("dictionary", dict);
		action->setCheckable(true);
		connect(action,SIGNAL(triggered()),SLOT(onChangeDictionary()));
		FDictMenu->addAction(action);
	}
	return true;
}

bool SpellChecker::isSpellEnabled() const
{
	return Options::node(OPV_MESSAGES_SPELL_ENABLED).value().toBool();
}

void SpellChecker::setSpellEnabled(bool AEnabled)
{
	Options::node(OPV_MESSAGES_SPELL_ENABLED).setValue(AEnabled);
}

void SpellChecker::onChangeSpellEnable()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
	{
		setSpellEnabled(action->isChecked());
	}
}

void SpellChecker::onChangeDictionary()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action && FSpellHighlighters.contains(FCurrentTextEdit))
	{
		action->setChecked(true);
		QString lang = action->property("dictionary").toString();
		SpellBackend::instance()->setLang(lang);
		FSpellHighlighters.value(FCurrentTextEdit)->rehighlight();
		Options::node(OPV_MESSAGES_SPELL_LANG).setValue(lang);
	}
}

void SpellChecker::onRepairWordUnderCursor()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action && FSpellHighlighters.contains(FCurrentTextEdit))
	{
		QTextCursor cursor = FCurrentTextEdit->textCursor();
		cursor.setPosition(FCurrentCursorPosition, QTextCursor::MoveAnchor);
		cursor.select(QTextCursor::WordUnderCursor);
		cursor.beginEditBlock();
		cursor.removeSelectedText();
		cursor.insertText(action->property("suggestion").toString());
		cursor.endEditBlock();
		FSpellHighlighters.value(FCurrentTextEdit)->rehighlightBlock(cursor.block());
	}
}

void SpellChecker::onAddUnknownWordToDictionary()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action && FSpellHighlighters.contains(FCurrentTextEdit))
	{
		QTextCursor cursor = FCurrentTextEdit->textCursor();
		cursor.setPosition(FCurrentCursorPosition, QTextCursor::MoveAnchor);
		cursor.select(QTextCursor::WordUnderCursor);
		SpellBackend::instance()->add(cursor.selectedText());
		FSpellHighlighters.value(FCurrentTextEdit)->rehighlightBlock(cursor.block());
	}
}

void SpellChecker::onEditWidgetCreated(IEditWidget *AWidget)
{
	QTextEdit *textEdit = AWidget->textEdit();
	textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(textEdit,SIGNAL(destroyed(QObject *)),SLOT(onTextEditDestroyed(QObject *)));
	connect(AWidget->instance(),SIGNAL(editContextMenu(const QPoint &, Menu *)),SLOT(onEditWidgetContextMenuRequested(const QPoint &, Menu *)));

	IMultiUserChatWindow *mucWindow = NULL;
	QWidget *parent = AWidget->instance()->parentWidget();
	while (mucWindow==NULL && parent!=NULL)
	{
		mucWindow = qobject_cast<IMultiUserChatWindow *>(parent);
		parent = parent->parentWidget();
	}
	SpellHighlighter *liter = new SpellHighlighter(AWidget->document(), mucWindow!=NULL ? mucWindow->multiUserChat() : NULL);
	liter->setEnabled(isSpellEnabled());
	FSpellHighlighters.insert(textEdit, liter);
}

void SpellChecker::onEditWidgetContextMenuRequested(const QPoint &APosition, Menu *AMenu)
{
	IEditWidget *editWidget = qobject_cast<IEditWidget *>(sender());
	if (editWidget)
	{
		FCurrentTextEdit = editWidget->textEdit();
		if (SpellBackend::instance()->available())
		{
			QTextCursor cursor = FCurrentTextEdit->cursorForPosition(APosition);
			FCurrentCursorPosition = cursor.position();
			cursor.select(QTextCursor::WordUnderCursor);
			const QString word = cursor.selectedText();

			if (isSpellEnabled() && !word.isEmpty() && !SpellBackend::instance()->isCorrect(word)) 
			{
				foreach(QString suggestion, SpellBackend::instance()->suggestions(word))
				{
					Action *suggestAction = new Action(AMenu);
					suggestAction->setText(suggestion);
					suggestAction->setProperty("suggestion", suggestion);
					connect(suggestAction,SIGNAL(triggered()),SLOT(onRepairWordUnderCursor()));
					AMenu->addAction(suggestAction,AG_EWCM_SPELLCHECKER_SUGGESTS);
				}

				if (SpellBackend::instance()->writable())
				{
					Action *appendAction = new Action(AMenu);
					appendAction->setText(tr("Add to Dictionary"));
					appendAction->setProperty("word",word);
					connect(appendAction,SIGNAL(triggered()),SLOT(onAddUnknownWordToDictionary()));
					AMenu->addAction(appendAction,AG_EWCM_SPELLCHECKER_SUGGESTS);
				}
			}
		}

		Action *enableAction = new Action(AMenu);
		enableAction->setText(tr("Spell Check"));
		enableAction->setCheckable(true);
		enableAction->setChecked(isSpellEnabled());
		connect(enableAction,SIGNAL(triggered()),SLOT(onChangeSpellEnable()));
		AMenu->addAction(enableAction,AG_EWCM_SPELLCHECKER_OPTIONS);

		if (!FDictMenu->isEmpty() && isSpellEnabled())
		{
			QString actualLang = SpellBackend::instance()->actuallLang();
			foreach(Action *action, FDictMenu->groupActions())
			{
				QString dict = action->property("dictionary").toString();
				action->setChecked(actualLang==dict || actualLang.contains(dict));
			}
			AMenu->addAction(FDictMenu->menuAction(),AG_EWCM_SPELLCHECKER_OPTIONS);
		}

		AMenu->popup(FCurrentTextEdit->mapToGlobal(APosition));
	}
}

void SpellChecker::onTextEditDestroyed(QObject *AObject)
{
	FSpellHighlighters.remove(AObject);
}

void SpellChecker::onOptionsOpened()
{
	SpellBackend::instance()->setLang(Options::node(OPV_MESSAGES_SPELL_LANG).value().toString());
	onOptionsChanged(Options::node(OPV_MESSAGES_SPELL_ENABLED));
}

void SpellChecker::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_SPELL_ENABLED)
	{
		foreach(SpellHighlighter *liter, FSpellHighlighters.values())
			liter->setEnabled(ANode.value().toBool());
	}
}

Q_EXPORT_PLUGIN2(plg_spellchecker, SpellChecker)
