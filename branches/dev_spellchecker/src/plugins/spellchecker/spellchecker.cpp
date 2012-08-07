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
	FDictMenu = new QMenu(tr("Dictionary"));

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
		QAction *action = new QAction(FDictMenu);
		action->setText(dict);
		action->setProperty("dictionary", dict);
		action->setCheckable(true);
		connect(action,SIGNAL(triggered()),SLOT(onChangeDictionary()));
		FDictMenu->addAction(action);
	}
	return true;
}

void SpellChecker::onChangeEnable()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
	{
		Options::node(OPV_MESSAGES_SPELL_ENABLED).setValue(action->isChecked());
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
	connect(textEdit, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onEditWidgetContextMenuRequested(const QPoint &)));

	IMultiUserChatWindow *mucWindow = NULL;
	QWidget *parent = AWidget->instance()->parentWidget();
	while (mucWindow==NULL && parent!=NULL)
	{
		mucWindow = qobject_cast<IMultiUserChatWindow *>(parent);
		parent = parent->parentWidget();
	}
	SpellHighlighter *liter = new SpellHighlighter(AWidget->document(), mucWindow!=NULL ? mucWindow->multiUserChat() : NULL);
	liter->setEnabled(Options::node(OPV_MESSAGES_SPELL_ENABLED).value().toBool());
	FSpellHighlighters.insert(textEdit, liter);
}

void SpellChecker::onEditWidgetContextMenuRequested(const QPoint &APos)
{
	FCurrentTextEdit = qobject_cast<QTextEdit *>(sender());
	if (FCurrentTextEdit)
	{
		QMenu *menu = FCurrentTextEdit->createStandardContextMenu();
		menu->setAttribute(Qt::WA_DeleteOnClose,true);
		
		QAction *beforeAction = menu->actions().value(0);
		if (SpellBackend::instance()->available())
		{
			QTextCursor cursor = FCurrentTextEdit->cursorForPosition(APos);
			FCurrentCursorPosition = cursor.position();
			cursor.select(QTextCursor::WordUnderCursor);
			const QString word = cursor.selectedText();

			if (!word.isEmpty() && !SpellBackend::instance()->isCorrect(word)) 
			{
				foreach(QString suggestion, SpellBackend::instance()->suggestions(word))
				{
					QAction *suggestAction = new QAction(menu);
					suggestAction->setText(suggestion);
					suggestAction->setProperty("suggestion", suggestion);
					connect(suggestAction,SIGNAL(triggered()),SLOT(onRepairWordUnderCursor()));
					menu->insertAction(beforeAction,suggestAction);
				}

				if (SpellBackend::instance()->writable())
				{
					QAction *appendAction = new QAction(menu);
					appendAction->setText(tr("Add to Dictionary"));
					appendAction->setProperty("word",word);
					connect(appendAction,SIGNAL(triggered()),SLOT(onAddUnknownWordToDictionary()));
					menu->insertAction(beforeAction,appendAction);
				}
			}
		}

		menu->insertSeparator(beforeAction);
		menu->addSeparator();

		QAction *enableAction = new QAction(menu);
		enableAction->setText(tr("Spell Check"));
		enableAction->setCheckable(true);
		enableAction->setChecked(Options::node(OPV_MESSAGES_SPELL_ENABLED).value().toBool());
		connect(enableAction,SIGNAL(triggered()),SLOT(onChangeEnable()));
		menu->addAction(enableAction);

		if (!FDictMenu->isEmpty())
		{
			QString actualLang = SpellBackend::instance()->actuallLang();
			foreach(QAction *action, FDictMenu->actions())
			{
				QString dict = action->property("dictionary").toString();
				action->setChecked(actualLang==dict || actualLang.contains(dict));
			}
			menu->addMenu(FDictMenu);
		}

		menu->popup(FCurrentTextEdit->mapToGlobal(APos));
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
