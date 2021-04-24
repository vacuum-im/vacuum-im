#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QApplication>
#include <QActionGroup>

#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>
#include <utils/logger.h>

#include "spellchecker.h"
#if defined(HAVE_MACSPELL)
#	include "macspellchecker.h"
#elif defined(HAVE_ENCHANT)
#	include "enchantchecker.h"
#elif defined(HAVE_ASPELL)
#	include "aspellchecker.h"
#elif defined(HAVE_HUNSPELL)
#	include "hunspellchecker.h"
#endif

#define MAX_CACHEDWORDS     2000
#define MAX_SUGGESTIONS     15
#define SPELLDICTS_DIR      "spelldicts"
#define PERSONALDICTS_DIR   "personal"

SpellChecker::SpellChecker()
{
	FPluginManager = NULL;
	FMessageWidgets = NULL;
	FCurrentTextEdit = NULL;
	FCurrentCursorPosition = 0;
	FCachedWords = new QHash<QString, bool>();
}

SpellChecker::~SpellChecker()
{

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
			connect(FMessageWidgets->instance(),SIGNAL(editWidgetCreated(IMessageEditWidget *)),SLOT(onEditWidgetCreated(IMessageEditWidget *)));
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FMessageWidgets != NULL;
}

bool SpellChecker::initObjects()
{
#if defined(HAVE_MACSPELL)
		FSpellBackend = new MacSpellChecker();
		Logger::writeLog(Logger::Info,"SpellBackend","MacSpell backend created");
#elif defined (HAVE_ENCHANT)
		FSpellBackend = new EnchantChecker();
		Logger::writeLog(Logger::Info,"SpellBackend","Enchant backend created");
#elif defined(HAVE_ASPELL)
		FSpellBackend = new ASpellChecker();
		Logger::writeLog(Logger::Info,"SpellBackend","Aspell backend created");
#elif defined(HAVE_HUNSPELL)
		FSpellBackend = new HunspellChecker();
		Logger::writeLog(Logger::Info,"SpellBackend","Hunspell backend created");
#else
		FSpellBackend = new SpellBackend();
		Logger::writeLog(Logger::Warning,"SpellBackend","Empty backend created");
#endif

	QDir dictsDir(FPluginManager->homePath());
	if (!dictsDir.exists(SPELLDICTS_DIR))
		dictsDir.mkdir(SPELLDICTS_DIR);
	dictsDir.cd(SPELLDICTS_DIR);
	FSpellBackend->setCustomDictPath(dictsDir.absolutePath());
	LOG_DEBUG(QString("Custom dictionary path set to=%1").arg(dictsDir.absolutePath()));

	if (!dictsDir.exists(PERSONALDICTS_DIR))
		dictsDir.mkdir(PERSONALDICTS_DIR);
	dictsDir.cd(PERSONALDICTS_DIR);
	FSpellBackend->setPersonalDictPath(dictsDir.absolutePath());
	LOG_DEBUG(QString("Personal dictionary path set to=%1").arg(dictsDir.absolutePath()));

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

bool SpellChecker::isSpellAvailable() const
{
	return FSpellBackend->available();
}

QList<QString> SpellChecker::availDictionaries() const
{
	return FSpellBackend->dictionaries();
}

QString SpellChecker::currentDictionary() const
{
	return FSpellBackend->actuallLang();
}

void SpellChecker::setCurrentDictionary(const QString &ADict)
{
	Options::node(OPV_MESSAGES_SPELL_LANG).setValue(ADict);
}

bool SpellChecker::isCorrectWord(const QString &AWord) const
{
	if (AWord.trimmed().isEmpty())
		return true;

	if (FCachedWords->contains(AWord))
		return FCachedWords->value(AWord);

	if (FCachedWords->size() >= MAX_CACHEDWORDS)
		FCachedWords->clear();
	bool correct = FSpellBackend->isCorrect(AWord);
	FCachedWords->insert(AWord, correct);
	return correct;
}

QList<QString> SpellChecker::wordSuggestions(const QString &AWord) const
{
	return FSpellBackend->suggestions(AWord);
}

bool SpellChecker::canAddWordToPersonalDict(const QString &AWord) const
{
	return FSpellBackend->writable() && FSpellBackend->canAdd(AWord);
}

void SpellChecker::addWordToPersonalDict(const QString &AWord)
{
	if (FSpellBackend->add(AWord))
	{
		FCachedWords->remove(AWord);
		rehightlightAll();
		emit wordAddedToPersonalDict(AWord);
	}
}

void SpellChecker::rehightlightAll()
{
	foreach(SpellHighlighter *hiliter, FSpellHighlighters)
		hiliter->rehighlight();
}

QString SpellChecker::dictionaryName(const QString &ADict) const
{
	QString name = ADict.left(ADict.indexOf('.'));
	if (name.size() >= 2)
	{
		QString localeName = name.size()>=5 && name.at(2)=='_' && name.at(3).isUpper() && name.at(4).isUpper() ? name.left(5) : name.left(2);

		QLocale locale(localeName);
		if (locale.language() > QLocale::C)
		{
			QString suffix = name.right(name.size()-localeName.size());
			if (!suffix.isEmpty() && !suffix.at(0).isLetterOrNumber())
				suffix.remove(0,1);

			name = QLocale::languageToString(locale.language());
			if (locale.country()>QLocale::AnyCountry && localeName.contains('_'))
				name += "/" + QLocale::countryToString(locale.country());
			if (!suffix.isEmpty())
				name += QString(" (%1)").arg(suffix);
		}
	}
	return name;
}

void SpellChecker::onChangeSpellEnable()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		setSpellEnabled(action->isChecked());
}

void SpellChecker::onChangeDictionary()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		setCurrentDictionary(action->property("dictionary").toString());
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
		addWordToPersonalDict(cursor.selectedText());
	}
}

void SpellChecker::onEditWidgetCreated(IMessageEditWidget *AWidget)
{
	QTextEdit *textEdit = AWidget->textEdit();
	textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(textEdit,SIGNAL(destroyed(QObject *)),SLOT(onTextEditDestroyed(QObject *)));
	connect(AWidget->instance(),SIGNAL(contextMenuRequested(const QPoint &, Menu *)),SLOT(onEditWidgetContextMenuRequested(const QPoint &, Menu *)));

	IMultiUserChatWindow *mucWindow = NULL;
	QWidget *parent = AWidget->instance()->parentWidget();
	while (mucWindow==NULL && parent!=NULL)
	{
		mucWindow = qobject_cast<IMultiUserChatWindow *>(parent);
		parent = parent->parentWidget();
	}
	SpellHighlighter *liter = new SpellHighlighter(this, AWidget->document(), mucWindow!=NULL ? mucWindow->multiUserChat() : NULL);
	liter->setEnabled(isSpellEnabled() && isSpellAvailable());
	FSpellHighlighters.insert(textEdit, liter);
}

void SpellChecker::onEditWidgetContextMenuRequested(const QPoint &APosition, Menu *AMenu)
{
	IMessageEditWidget *editWidget = qobject_cast<IMessageEditWidget *>(sender());
	if (editWidget)
	{
		FCurrentTextEdit = editWidget->textEdit();
		if (isSpellEnabled() && isSpellAvailable())
		{
			QTextCursor cursor = FCurrentTextEdit->cursorForPosition(APosition);
			FCurrentCursorPosition = cursor.position();
			cursor.select(QTextCursor::WordUnderCursor);
			const QString word = cursor.selectedText();

			if (!isCorrectWord(word))
			{
				QList<QString> suggests = wordSuggestions(word);
				for(int i=0; i<suggests.count() && i<MAX_SUGGESTIONS; i++)
				{
					Action *suggestAction = new Action(AMenu);
					suggestAction->setText(suggests.at(i));
					suggestAction->setProperty("suggestion", suggests.at(i));
					connect(suggestAction,SIGNAL(triggered()),SLOT(onRepairWordUnderCursor()));
					AMenu->addAction(suggestAction,AG_MWEWCM_SPELLCHECKER_SUGGESTS);
				}

				if (canAddWordToPersonalDict(word))
				{
					Action *appendAction = new Action(AMenu);
					appendAction->setText(tr("Add '%1' to Dictionary").arg(word));
					appendAction->setProperty("word",word);
					connect(appendAction,SIGNAL(triggered()),SLOT(onAddUnknownWordToDictionary()));
					AMenu->addAction(appendAction,AG_MWEWCM_SPELLCHECKER_SUGGESTS);
				}
			}
		}

		Action *enableAction = new Action(AMenu);
		enableAction->setText(tr("Spell Check"));
		enableAction->setCheckable(true);
		enableAction->setChecked(isSpellEnabled() && isSpellAvailable());
		enableAction->setEnabled(isSpellAvailable());
		connect(enableAction,SIGNAL(triggered()),SLOT(onChangeSpellEnable()));
		AMenu->addAction(enableAction,AG_MWEWCM_SPELLCHECKER_OPTIONS);

		if (isSpellEnabled())
		{
			Menu *dictsMenu = new Menu(AMenu);
			dictsMenu->setTitle(tr("Dictionary"));
			AMenu->addAction(dictsMenu->menuAction(),AG_MWEWCM_SPELLCHECKER_OPTIONS);
			QActionGroup *dictGroup = new QActionGroup(dictsMenu);

			QString curDict = currentDictionary();
			foreach(const QString &dict, availDictionaries())
			{
				Action *action = new Action(dictsMenu);
				action->setText(dictionaryName(dict));
				action->setProperty("dictionary", dict);
				action->setCheckable(true);
				action->setChecked(curDict==dict);
				dictGroup->addAction(action);
				connect(action,SIGNAL(triggered()),SLOT(onChangeDictionary()));
				dictsMenu->addAction(action,AG_DEFAULT,true);
			}
			dictsMenu->setEnabled(!dictsMenu->isEmpty());
		}
	}
}

void SpellChecker::onTextEditDestroyed(QObject *AObject)
{
	FSpellHighlighters.remove(AObject);
}

void SpellChecker::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_MESSAGES_SPELL_ENABLED));
	onOptionsChanged(Options::node(OPV_MESSAGES_SPELL_LANG));
}

void SpellChecker::onOptionsChanged(const OptionsNode &ANode)
{
	FCachedWords->clear();
	if (ANode.path() == OPV_MESSAGES_SPELL_ENABLED)
	{
		bool enabled = ANode.value().toBool();
		LOG_INFO(QString("Spell check enable changed to=%1").arg(enabled));
		foreach(SpellHighlighter *liter, FSpellHighlighters.values())
			liter->setEnabled(enabled);
		emit spellEnableChanged(enabled);
	}
	else if (ANode.path() == OPV_MESSAGES_SPELL_LANG)
	{
		if (isSpellEnabled())
		{
			QString fullDict = ANode.value().toString();
			QString partDict = fullDict.split('_').value(0);
			QList<QString> availDicts = availDictionaries();
			QString dict = availDicts.contains(fullDict) ? fullDict : partDict;
			if (availDicts.contains(dict))
			{
				LOG_INFO(QString("Spell check language changed to=%1").arg(dict));
				FSpellBackend->setLang(dict);
				emit currentDictionaryChanged(currentDictionary());
				rehightlightAll();
			}
		}
	}
}
