#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QApplication>

#include "spellbackend.h"
#include "spellchecker.h"

#define MAX_SUGGESTIONS     15
#define SPELLDICTS_DIR      "spelldicts"
#define PERSONALDICTS_DIR   "personal"

SpellChecker::SpellChecker()
{
	FMessageWidgets = NULL;

	FCurrentTextEdit = NULL;
	FCurrentCursorPosition = 0;
}

SpellChecker::~SpellChecker()
{
	SpellBackend::destroyInstance();
}

void SpellChecker::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Spell Checker");
	APluginInfo->description = tr("Highlights words that may not be spelled correctly");
	APluginInfo->version = "1.0";
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
		if (isSpellEnabled() && SpellBackend::instance()->available())
		{
			QTextCursor cursor = FCurrentTextEdit->cursorForPosition(APosition);
			FCurrentCursorPosition = cursor.position();
			cursor.select(QTextCursor::WordUnderCursor);
			const QString word = cursor.selectedText();

			if (!word.isEmpty() && !SpellBackend::instance()->isCorrect(word)) 
			{
				QList<QString> suggests = SpellBackend::instance()->suggestions(word);
				for(int i=0; i<suggests.count() && i<MAX_SUGGESTIONS; i++)
				{
					Action *suggestAction = new Action(AMenu);
					suggestAction->setText(suggests.at(i));
					suggestAction->setProperty("suggestion", suggests.at(i));
					connect(suggestAction,SIGNAL(triggered()),SLOT(onRepairWordUnderCursor()));
					AMenu->addAction(suggestAction,AG_EWCM_SPELLCHECKER_SUGGESTS);
				}

				if (SpellBackend::instance()->canAdd(word))
				{
					Action *appendAction = new Action(AMenu);
					appendAction->setText(tr("Add '%1' to Dictionary").arg(word));
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

		if (isSpellEnabled())
		{
			Menu *dictsMenu = new Menu(AMenu);
			dictsMenu->setTitle(tr("Dictionary"));
			AMenu->addAction(dictsMenu->menuAction(),AG_EWCM_SPELLCHECKER_OPTIONS);

			QString actualLang = SpellBackend::instance()->actuallLang();
			foreach(QString dict, SpellBackend::instance()->dictionaries())
			{
				Action *action = new Action(dictsMenu);
				QLocale locale(dict);
				if (locale.country()>QLocale::AnyCountry && dict.contains('_'))
					action->setText(QString("%1 (%2)").arg(QLocale::languageToString(locale.language()),QLocale::countryToString(locale.country())));
				else if (locale.language() > QLocale::C)
					action->setText(QLocale::languageToString(locale.language()));
				else
					action->setText(dict);
				action->setProperty("dictionary", dict);
				action->setCheckable(true);
				action->setChecked(actualLang==dict);
				connect(action,SIGNAL(triggered()),SLOT(onChangeDictionary()));
				dictsMenu->addAction(action,AG_DEFAULT,true);
			}
			dictsMenu->setEnabled(!dictsMenu->isEmpty());
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
