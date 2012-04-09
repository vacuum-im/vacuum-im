#include "consolewidget.h"

#include <QRegExp>
#include <QLineEdit>
#include <QInputDialog>

ConsoleWidget::ConsoleWidget(IPluginManager *APluginManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_CONSOLE,0,0,"windowIcon");

	FXmppStreams = NULL;
	FStanzaProcessor = NULL;

	ui.cmbStreamJid->addItem(tr("<All Streams>"));
	initialize(APluginManager);

	if (!Options::isNull())
		onOptionsOpened();

	connect(ui.tlbAddCondition,SIGNAL(clicked()),SLOT(onAddConditionClicked()));
	connect(ui.tlbRemoveCondition,SIGNAL(clicked()),SLOT(onRemoveConditionClicked()));
	connect(ui.tlbClearCondition,SIGNAL(clicked()),ui.ltwConditions,SLOT(clear()));
	connect(ui.cmbCondition->lineEdit(),SIGNAL(returnPressed()),SLOT(onAddConditionClicked()));

	connect(ui.tlbAddContext,SIGNAL(clicked()),SLOT(onAddContextClicked()));
	connect(ui.tlbRemoveContext,SIGNAL(clicked()),SLOT(onRemoveContextClicked()));
	connect(ui.cmbContext,SIGNAL(currentIndexChanged(int)),SLOT(onContextChanged(int)));

	connect(ui.tlbSendXML,SIGNAL(clicked()),SLOT(onSendXMLClicked()));
	connect(ui.tlbClearConsole,SIGNAL(clicked()),ui.tedConsole,SLOT(clear()));
	connect(ui.chbWordWrap,SIGNAL(stateChanged(int)),SLOT(onWordWrapStateChanged(int)));
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
		ui.cmbStreamJid->setCurrentIndex(ui.cmbStreamJid->findText(streamJid));

	ui.ltwConditions->clear();
	ui.ltwConditions->addItems(node.value("conditions").toStringList());

	ui.chbWordWrap->setChecked(node.value("word-wrap").toBool());
	ui.chbHilightXML->setCheckState((Qt::CheckState)node.value("highlight-xml").toInt());

	if (!restoreGeometry(Options::fileValue("console.context.window-geometry",AContextId.toString()).toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(640,640),this));
	ui.sptHSplitter->restoreState(Options::fileValue("console.context.hsplitter-state",AContextId.toString()).toByteArray());
	ui.sptVSplitter->restoreState(Options::fileValue("console.context.vsplitter-state",AContextId.toString()).toByteArray());

	setWindowTitle(tr("XML Console - %1").arg(node.value("name").toString()));
}

void ConsoleWidget::saveContext(const QUuid &AContextId)
{
	OptionsNode node = Options::node(OPV_CONSOLE_CONTEXT_ITEM, AContextId.toString());

	node.setValue(ui.cmbStreamJid->currentIndex()>0 ? ui.cmbStreamJid->currentText() : QString::null,"streamjid");

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
#if QT_VERSION <= 0x040503
		{ "\\sxmlns\\s?=\\s?\"(.+)\"",          " <u><span style='color:darkred;'>xmlns</span>=\"<i>\\1</i>\"</u>",   true       },   //xmlns
		{ "\\s([\\w:-]+\\s?)=\\s?\"",           " <span style='color:darkred;'>\\1</span>=\"",                        false      }    //attribute
#else
		{ "\\sxmlns\\s?=\\s?&quot;(.+)&quot;",  " <u><span style='color:darkred;'>xmlns</span>=\"<i>\\1</i>\"</u>",   true       },   //xmlns
		{ "\\s([\\w:-]+\\s?)=\\s?&quot;",       " <span style='color:darkred;'>\\1</span>=\"",                        false      }    //attribute
#endif
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
	Jid streamJid = ui.cmbStreamJid->currentIndex()>0 ? ui.cmbStreamJid->itemText(ui.cmbStreamJid->currentIndex()) : QString::null;
	if (streamJid.isEmpty() || streamJid==AXmppStream->streamJid())
	{
		Stanza stanza(AElem);
		bool accepted = FStanzaProcessor==NULL || ui.ltwConditions->count()==0;
		for (int i=0; !accepted && i<ui.ltwConditions->count(); i++)
			accepted = FStanzaProcessor->checkStanza(stanza,ui.ltwConditions->item(i)->text());

		if (accepted)
		{
			static QString sended =   Qt::escape(">>>>") + " <b>%1</b> %2 +%3 " + Qt::escape(">>>>");
			static QString received = Qt::escape("<<<<") + " <b>%1</b> %2 +%3 " + Qt::escape("<<<<");

			int delta = FTimePoint.isValid() ? FTimePoint.msecsTo(QTime::currentTime()) : 0;
			FTimePoint = QTime::currentTime();
			QString caption = (ASended ? sended : received).arg(Qt::escape(AXmppStream->streamJid().uFull())).arg(FTimePoint.toString()).arg(delta);
			ui.tedConsole->append(caption);

			QString xml = stanza.toString(2);
			hidePasswords(xml);
			xml = "<pre>"+Qt::escape(xml).replace('\n',"<br>")+"</pre>";
			if (ui.chbHilightXML->checkState() == Qt::Checked)
				colorXml(xml);
			else if (ui.chbHilightXML->checkState()==Qt::PartiallyChecked && xml.size() < 5000)
				colorXml(xml);
			ui.tedConsole->append(xml);
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
			ui.tedConsole->append("<b>"+tr("Start sending user stanza...")+"</b><br>");
			foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
				if (ui.cmbStreamJid->currentIndex()==0 || stream->streamJid()==ui.cmbStreamJid->currentText())
					stream->sendStanza(stanza);
			ui.tedConsole->append("<b>"+tr("User stanza sent.")+"</b><br>");
		}
		else
		{
			ui.tedConsole->append("<b>"+tr("Stanza is not well formed.")+"</b><br>");
		}
	}
	else
	{
		ui.tedConsole->append("<b>"+tr("XML is not well formed.")+"</b><br>");
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

void ConsoleWidget::onWordWrapStateChanged(int AState)
{
	ui.tedConsole->setLineWrapMode(AState == Qt::Checked ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void ConsoleWidget::onStreamCreated(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->addItem(AXmppStream->streamJid().uFull());
	AXmppStream->insertXmppStanzaHandler(XSHO_CONSOLE,this);
}

void ConsoleWidget::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	int index = ui.cmbStreamJid->findText(ABefore.uFull());
	if (index >= 0)
	{
		ui.cmbStreamJid->removeItem(index);
		ui.cmbStreamJid->addItem(AXmppStream->streamJid().uFull());
	}
}

void ConsoleWidget::onStreamDestroyed(IXmppStream *AXmppStream)
{
	ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findText(AXmppStream->streamJid().uFull()));
	AXmppStream->removeXmppStanzaHandler(XSHO_CONSOLE,this);
}

void ConsoleWidget::onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle)
{
	Q_UNUSED(AHandleId);
	foreach(QString condition, AHandle.conditions)
		if (ui.cmbCondition->findText(condition) < 0)
			ui.cmbCondition->addItem(condition);
}

void ConsoleWidget::onOptionsOpened()
{
	ui.cmbContext->clear();
	foreach(QString contextId, Options::node(OPV_CONSOLE_ROOT).childNSpaces("context"))
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
