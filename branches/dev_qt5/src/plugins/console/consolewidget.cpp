#include "consolewidget.h"

#include <QRegExp>
#include <QLineEdit>
#include <QInputDialog>

#define MAX_HILIGHT_ITEMS            10
#define TEXT_SEARCH_TIMEOUT          500

ConsoleWidget::ConsoleWidget(IPluginManager *APluginManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_CONSOLE,0,0,"windowIcon");

	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	
	FSearchMoveCursor = false;

	ui.cmbStreamJid->addItem(tr("<All Streams>"));
	initialize(APluginManager);

	if (!Options::isNull())
		onOptionsOpened();

	ui.cmbCondition->view()->setTextElideMode(Qt::ElideNone);

	QPalette palette = ui.tbrConsole->palette();
	palette.setColor(QPalette::Inactive,QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
	palette.setColor(QPalette::Inactive,QPalette::HighlightedText,palette.color(QPalette::Active,QPalette::HighlightedText));
	ui.tbrConsole->setPalette(palette);

	FTextHilightTimer.setSingleShot(true);
	connect(&FTextHilightTimer,SIGNAL(timeout()),SLOT(onTextHilightTimerTimeout()));
	connect(ui.tbrConsole,SIGNAL(visiblePositionBoundaryChanged()),SLOT(onTextVisiblePositionBoundaryChanged()));

	ui.lneTextSearch->setPlaceholderText(tr("Search"));
	ui.tlbTextSearchNext->setIcon(style()->standardIcon(QStyle::SP_ArrowDown, NULL, this));
	ui.tlbTextSearchPrev->setIcon(style()->standardIcon(QStyle::SP_ArrowUp, NULL, this));
	connect(ui.tlbTextSearchNext,SIGNAL(clicked()),SLOT(onTextSearchNextClicked()));
	connect(ui.tlbTextSearchPrev,SIGNAL(clicked()),SLOT(onTextSearchPreviousClicked()));
	connect(ui.lneTextSearch,SIGNAL(searchStart()),SLOT(onTextSearchStart()));
	connect(ui.lneTextSearch,SIGNAL(searchNext()),SLOT(onTextSearchNextClicked()));
	connect(ui.lneTextSearch,SIGNAL(textChanged(const QString &)),SLOT(onTextSearchTextChanged(const QString &)));

	connect(ui.tlbAddCondition,SIGNAL(clicked()),SLOT(onAddConditionClicked()));
	connect(ui.tlbRemoveCondition,SIGNAL(clicked()),SLOT(onRemoveConditionClicked()));
	connect(ui.tlbClearCondition,SIGNAL(clicked()),ui.ltwConditions,SLOT(clear()));
	connect(ui.cmbCondition->lineEdit(),SIGNAL(returnPressed()),SLOT(onAddConditionClicked()));

	connect(ui.tlbAddContext,SIGNAL(clicked()),SLOT(onAddContextClicked()));
	connect(ui.tlbRemoveContext,SIGNAL(clicked()),SLOT(onRemoveContextClicked()));
	connect(ui.cmbContext,SIGNAL(currentIndexChanged(int)),SLOT(onContextChanged(int)));

	connect(ui.tlbSendXML,SIGNAL(clicked()),SLOT(onSendXMLClicked()));
	connect(ui.tlbClearConsole,SIGNAL(clicked()),ui.tbrConsole,SLOT(clear()));
	connect(ui.tlbClearConsole,SIGNAL(clicked()),SLOT(onTextSearchStart()));
	connect(ui.chbWordWrap,SIGNAL(toggled(bool)),SLOT(onWordWrapButtonToggled(bool)));
}

ConsoleWidget::~ConsoleWidget()
{
	foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
		stream->removeXmppStanzaHandler(XSHO_CONSOLE,this);
	if (!Options::isNull())
		onOptionsClosed();
}

bool ConsoleWidget::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AOrder == XSHO_CONSOLE)
		showElement(AXmppStream,AStanza.element(),false);
	return false;
}

bool ConsoleWidget::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AOrder == XSHO_CONSOLE)
		showElement(AXmppStream,AStanza.element(),true);
	return false;
}

void ConsoleWidget::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			foreach(IXmppStream *stream, FXmppStreams->xmppStreams()) {
				onStreamCreated(stream); }
			connect(FXmppStreams->instance(), SIGNAL(created(IXmppStream *)), SLOT(onStreamCreated(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)), SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(), SIGNAL(streamDestroyed(IXmppStream *)), SLOT(onStreamDestroyed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
		if (FStanzaProcessor)
		{
			foreach(int shandleId, FStanzaProcessor->stanzaHandles()) {
				onStanzaHandleInserted(shandleId,FStanzaProcessor->stanzaHandle(shandleId)); }
			ui.cmbCondition->clearEditText();

			connect(FStanzaProcessor->instance(),SIGNAL(stanzaHandleInserted(int, const IStanzaHandle &)),
			        SLOT(onStanzaHandleInserted(int, const IStanzaHandle &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
}

void ConsoleWidget::loadContext(const QUuid &AContextId)
{
	OptionsNode node = Options::node(OPV_CONSOLE_CONTEXT_ITEM, AContextId.toString());

	QString streamJid = node.value("streamjid").toString();
	if (streamJid.isEmpty())
		ui.cmbStreamJid->setCurrentIndex(0);
	else
		ui.cmbStreamJid->setCurrentIndex(ui.cmbStreamJid->findData(streamJid));

	ui.ltwConditions->clear();
	ui.ltwConditions->addItems(node.value("conditions").toStringList());

	ui.chbWordWrap->setChecked(node.value("word-wrap").toBool());
	ui.chbHilightXML->setCheckState((Qt::CheckState)node.value("highlight-xml").toInt());
	onWordWrapButtonToggled(ui.chbWordWrap->isChecked());

	if (!restoreGeometry(Options::fileValue("console.context.window-geometry",AContextId.toString()).toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(640,640),this));
	ui.sptHSplitter->restoreState(Options::fileValue("console.context.hsplitter-state",AContextId.toString()).toByteArray());
	ui.sptVSplitter->restoreState(Options::fileValue("console.context.vsplitter-state",AContextId.toString()).toByteArray());

	setWindowTitle(tr("XML Console - %1").arg(node.value("name").toString()));
}

void ConsoleWidget::saveContext(const QUuid &AContextId)
{
	OptionsNode node = Options::node(OPV_CONSOLE_CONTEXT_ITEM, AContextId.toString());
	node.setValue(ui.cmbStreamJid->currentIndex()>0 ? ui.cmbStreamJid->itemData(ui.cmbStreamJid->currentIndex()).toString() : QString::null,"streamjid");

	QStringList conditions;
	for (int i=0; i<ui.ltwConditions->count(); i++)
		conditions.append(ui.ltwConditions->item(i)->text());
	node.setValue(conditions,"conditions");

	node.setValue(ui.chbWordWrap->isChecked(),"word-wrap");
	node.setValue(ui.chbHilightXML->checkState(),"highlight-xml");

	Options::setFileValue(saveGeometry(),"console.context.window-geometry",AContextId.toString());
	Options::setFileValue(ui.sptHSplitter->saveState(),"console.context.hsplitter-state",AContextId.toString());
	Options::setFileValue(ui.sptVSplitter->saveState(),"console.context.vsplitter-state",AContextId.toString());
}

void ConsoleWidget::colorXml(QString &AXml) const
{
	static const struct { const char *regexp ; const char *replace; bool minimal;} changes[] =
	{
		{ "\n",                                 "<br>"                                          ,                     false      },
		{ "&lt;([\\w:-]+)((\\s|/|&gt))",        "&lt;<span style='color:navy;'>\\1</span>\\2"   ,                     false      },   //open tagName
		{ "&lt;/([\\w:-]+)&gt;",                "&lt;/<span style='color:navy;'>\\1</span>&gt;" ,                     false      },   //close tagName
		{ "\\sxmlns\\s?=\\s?&quot;(.+)&quot;",  " <u><span style='color:darkred;'>xmlns</span>=\"<i>\\1</i>\"</u>",   true       },   //xmlns
		{ "\\s([\\w:-]+\\s?)=\\s?&quot;",       " <span style='color:darkred;'>\\1</span>=\"",                        false      }    //attribute
	};
	static const int changesCount = sizeof(changes)/sizeof(changes[0]);

	for (int i=0; i<changesCount; i++)
	{
		QRegExp regexp(changes[i].regexp);
		regexp.setMinimal(changes[i].minimal);
		AXml.replace(regexp,changes[i].replace);
	}
}

void ConsoleWidget::hidePasswords(QString &AXml) const
{
	static const QRegExp passRegExp("<password>.*</password>", Qt::CaseInsensitive);
	static const QString passNewStr = "<password>[password]</password>";
	AXml.replace(passRegExp,passNewStr);
}

void ConsoleWidget::showElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended)
{
	Jid streamJid = ui.cmbStreamJid->currentIndex()>0 ? ui.cmbStreamJid->itemData(ui.cmbStreamJid->currentIndex()).toString() : QString::null;
	if (streamJid.isEmpty() || streamJid==AXmppStream->streamJid())
	{
		Stanza stanza(AElem);
		bool accepted = FStanzaProcessor==NULL || ui.ltwConditions->count()==0;
		for (int i=0; !accepted && i<ui.ltwConditions->count(); i++)
			accepted = FStanzaProcessor->checkStanza(stanza,ui.ltwConditions->item(i)->text());

		if (accepted)
		{
			static QString sended =   QString(">>>>").toHtmlEscaped() + " <b>%1</b> %2 +%3 " + QString(">>>>").toHtmlEscaped();
			static QString received = QString("<<<<").toHtmlEscaped() + " <b>%1</b> %2 +%3 " + QString("<<<<").toHtmlEscaped();

			int delta = FTimePoint.isValid() ? FTimePoint.msecsTo(QTime::currentTime()) : 0;
			FTimePoint = QTime::currentTime();
			QString caption = (ASended ? sended : received).arg(AXmppStream->streamJid().uFull().toHtmlEscaped()).arg(FTimePoint.toString()).arg(delta);
			ui.tbrConsole->append(caption);

			QString xml = stanza.toString(2);
			hidePasswords(xml);
			xml = "<pre>"+xml.toHtmlEscaped().replace('\n',"<br>")+"</pre>";
			if (ui.chbHilightXML->checkState() == Qt::Checked)
				colorXml(xml);
			else if (ui.chbHilightXML->checkState()==Qt::PartiallyChecked && xml.size() < 5000)
				colorXml(xml);
			ui.tbrConsole->append(xml);

			ui.lneTextSearch->restartTimeout(ui.lneTextSearch->startSearchTimeout());
		}
	}
}

void ConsoleWidget::onAddConditionClicked()
{
	if (!ui.cmbCondition->currentText().isEmpty() && ui.ltwConditions->findItems(ui.cmbCondition->currentText(),Qt::MatchExactly).isEmpty())
	{
		QListWidgetItem *item = new QListWidgetItem(ui.cmbCondition->currentText());
		item->setToolTip(ui.cmbCondition->currentText());
		ui.ltwConditions->addItem(item);
		ui.cmbCondition->clearEditText();
	}
}

void ConsoleWidget::onRemoveConditionClicked()
{
	if (ui.ltwConditions->currentRow()>=0)
		delete ui.ltwConditions->takeItem(ui.ltwConditions->currentRow());
}

void ConsoleWidget::onSendXMLClicked()
{
	QDomDocument doc;
	if (FXmppStreams!=NULL && doc.setContent(ui.tedSendXML->toPlainText(),true))
	{
		Stanza stanza(doc.documentElement());
		if (stanza.isValid())
		{
			ui.tbrConsole->append("<b>"+tr("Start sending user stanza...")+"</b><br>");
			foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
				if (ui.cmbStreamJid->currentIndex()==0 || stream->streamJid()==ui.cmbStreamJid->itemData(ui.cmbStreamJid->currentIndex()).toString())
					stream->sendStanza(stanza);
			ui.tbrConsole->append("<b>"+tr("User stanza sent.")+"</b><br>");
		}
		else
		{
			ui.tbrConsole->append("<b>"+tr("Stanza is not well formed.")+"</b><br>");
		}
	}
	else
	{
		ui.tbrConsole->append("<b>"+tr("XML is not well formed.")+"</b><br>");
	}
}

void ConsoleWidget::onAddContextClicked()
{
	QString name = QInputDialog::getText(this,tr("New Context"),tr("Enter context name"));
	if (!name.isNull())
	{
		QUuid newId = QUuid::createUuid();
		Options::node(OPV_CONSOLE_CONTEXT_ITEM,newId.toString()).setValue(name,"name");
		ui.cmbContext->addItem(name,newId.toString());
		ui.cmbContext->setCurrentIndex(ui.cmbContext->findData(newId.toString()));
	}
}

void ConsoleWidget::onRemoveContextClicked()
{
	QUuid oldId = ui.cmbContext->itemData(ui.cmbContext->currentIndex()).toString();
	if (!oldId.isNull())
	{
		ui.cmbContext->removeItem(ui.cmbContext->findData(oldId.toString()));
		Options::node(OPV_CONSOLE_ROOT).removeChilds("context",oldId.toString());
	}
}

void ConsoleWidget::onContextChanged(int AIndex)
{
	saveContext(FContext);
	FContext = ui.cmbContext->itemData(AIndex).toString();
	loadContext(FContext);
}

void ConsoleWidget::onWordWrapButtonToggled(bool AChecked)
{
	ui.tbrConsole->setLineWrapMode(AChecked ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void ConsoleWidget::onTextHilightTimerTimeout()
{
	if (FSearchResults.count() > MAX_HILIGHT_ITEMS)
	{
		QList<QTextEdit::ExtraSelection> selections;
		QPair<int,int> boundary = ui.tbrConsole->visiblePositionBoundary();
		for (QMap<int, QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.lowerBound(boundary.first); it!=FSearchResults.constEnd() && it.key()<boundary.second; ++it)
			selections.append(it.value());
		ui.tbrConsole->setExtraSelections(selections);
	}
	else
	{
		ui.tbrConsole->setExtraSelections(FSearchResults.values());
	}
}

void ConsoleWidget::onTextVisiblePositionBoundaryChanged()
{
	FTextHilightTimer.start(0);
}

void ConsoleWidget::onTextSearchStart()
{
	FSearchResults.clear();
	if (!ui.lneTextSearch->text().isEmpty())
	{
		QTextCursor cursor(ui.tbrConsole->document());
		do {
			cursor = ui.tbrConsole->document()->find(ui.lneTextSearch->text(),cursor,0);
			if (!cursor.isNull())
			{
				QTextEdit::ExtraSelection selection;
				selection.cursor = cursor;
				selection.format = cursor.charFormat();
				selection.format.setBackground(Qt::yellow);
				FSearchResults.insert(cursor.position(),selection);
				cursor.clearSelection();
			}
		} while (!cursor.isNull());
	}

	if (!FSearchResults.isEmpty())
	{
		if (FSearchMoveCursor)
		{
			ui.tbrConsole->setTextCursor(FSearchResults.lowerBound(0)->cursor);
			ui.tbrConsole->ensureCursorVisible();
		}
	}
	else
	{
		QTextCursor cursor = ui.tbrConsole->textCursor();
		if (cursor.hasSelection())
		{
			cursor.clearSelection();
			ui.tbrConsole->setTextCursor(cursor);
		}
	}
	FSearchMoveCursor = false;

	if (!ui.lneTextSearch->text().isEmpty() && FSearchResults.isEmpty())
	{
		QPalette palette = this->palette();
		palette.setColor(QPalette::Active,QPalette::Base,QColor("orangered"));
		palette.setColor(QPalette::Active,QPalette::Text,Qt::white);
		ui.lneTextSearch->setPalette(palette);
	}
	else
	{
		ui.lneTextSearch->setPalette(QPalette());
	}

	ui.tlbTextSearchNext->setEnabled(!FSearchResults.isEmpty());
	ui.tlbTextSearchPrev->setEnabled(!FSearchResults.isEmpty());

	FTextHilightTimer.start(0);
}

void ConsoleWidget::onTextSearchNextClicked()
{
	QMap<int,QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.upperBound(ui.tbrConsole->textCursor().position());
	if (it != FSearchResults.constEnd())
	{
		ui.tbrConsole->setTextCursor(it->cursor);
		ui.tbrConsole->ensureCursorVisible();
	}
}

void ConsoleWidget::onTextSearchPreviousClicked()
{
	QMap<int,QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.lowerBound(ui.tbrConsole->textCursor().position());
	if (--it != FSearchResults.constEnd())
	{
		ui.tbrConsole->setTextCursor(it->cursor);
		ui.tbrConsole->ensureCursorVisible();
	}
}

void ConsoleWidget::onTextSearchTextChanged(const QString &AText)
{
	Q_UNUSED(AText);
	FSearchMoveCursor = true;
}

void ConsoleWidget::onStreamCreated(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->addItem(AXmppStream->streamJid().uFull(),AXmppStream->streamJid().pFull());
	AXmppStream->insertXmppStanzaHandler(XSHO_CONSOLE,this);
}

void ConsoleWidget::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	int index = ui.cmbStreamJid->findData(ABefore.pFull());
	if (index >= 0)
	{
		ui.cmbStreamJid->setItemText(index,AXmppStream->streamJid().uFull());
		ui.cmbStreamJid->setItemData(index,AXmppStream->streamJid().pFull());
	}
}

void ConsoleWidget::onStreamDestroyed(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findData(AXmppStream->streamJid().pFull()));
	AXmppStream->removeXmppStanzaHandler(XSHO_CONSOLE,this);
}

void ConsoleWidget::onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle)
{
	Q_UNUSED(AHandleId);
	foreach(const QString &condition, AHandle.conditions)
		if (ui.cmbCondition->findText(condition) < 0)
			ui.cmbCondition->addItem(condition);
}

void ConsoleWidget::onOptionsOpened()
{
	ui.cmbContext->clear();
	foreach(const QString &contextId, Options::node(OPV_CONSOLE_ROOT).childNSpaces("context"))
		ui.cmbContext->addItem(Options::node(OPV_CONSOLE_CONTEXT_ITEM,contextId).value("name").toString(),contextId);

	FContext = QUuid();
	if (ui.cmbContext->count() == 0)
		ui.cmbContext->addItem(Options::node(OPV_CONSOLE_CONTEXT_ITEM,FContext.toString()).value("name").toString(),FContext.toString());
	loadContext(FContext);
}

void ConsoleWidget::onOptionsClosed()
{
	saveContext(FContext);
	ui.cmbContext->clear();
}
