#include "consolewidget.h"

#include <QRegExp>
#include <QLineEdit>

#define CONSOLE_UUID  "{2572D474-5F3E-8d24-B10A-BAA57C2BC693}"

#define SVN_CONTEXT                   "context[]"
#define SVN_CONTEXT_STREAM            SVN_CONTEXT":stream"
#define SVN_CONTEXT_CONDITIONS        SVN_CONTEXT":conditions"
#define SVN_CONTEXT_COLORXML          SVN_CONTEXT":colorXML"
#define SBD_CONTEXT_SENDXML           "[%1]:sendXML"
#define SBD_CONTEXT_GEOMETRY          "[%1]:geometry"
#define SBD_CONTEXT_HSPLITTER         "[%1]:hsplitter"
#define SBD_CONTEXT_VSPLITTER         "[%1]:vsplitter"

ConsoleWidget::ConsoleWidget(IPluginManager *APluginManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_CONSOLE,0,0,"windowIcon");

  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;

  ui.cmbStreamJid->addItem(tr("<All Streams>"));

  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
        onStreamCreated(stream);
      connect(FXmppStreams->instance(), SIGNAL(created(IXmppStream *)), SLOT(onStreamCreated(IXmppStream *)));
      connect(FXmppStreams->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)), 
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
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

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      if (FSettingsPlugin->isProfileOpened())
      {
        onSettingsOpened();
      }
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  onLoadContextClicked();

  connect(ui.tlbClearConsole,SIGNAL(clicked()),ui.tedConsole,SLOT(clear()));
  connect(ui.tlbAddCondition,SIGNAL(clicked()),SLOT(onAddConditionClicked()));
  connect(ui.tlbRemoveCondition,SIGNAL(clicked()),SLOT(onRemoveConditionClicked()));
  connect(ui.tlbClearCondition,SIGNAL(clicked()),ui.ltwConditions,SLOT(clear()));
  connect(ui.cmbCondition->lineEdit(),SIGNAL(returnPressed()),SLOT(onAddConditionClicked()));
  connect(ui.tlbSendXML,SIGNAL(clicked()),SLOT(onSendXMLClicked()));
  connect(ui.tlbLoadContext,SIGNAL(clicked()),SLOT(onLoadContextClicked()));
  connect(ui.tlbSaveContext,SIGNAL(clicked()),SLOT(onSaveContextClicked()));
  connect(ui.tlbDeleteContext,SIGNAL(clicked()),SLOT(onDeleteContextClicked()));
  connect(ui.chbWordWrap,SIGNAL(stateChanged(int)),SLOT(onWordWrapStateChanged(int)));
}

ConsoleWidget::~ConsoleWidget()
{
  foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
    stream->removeXmppStanzaHandler(this, XSHO_CONSOLE);
}

bool ConsoleWidget::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
  if (AOrder == XSHO_CONSOLE)
  {
    showElement(AXmppStream,AStanza.element(),false);
  }
  return false;
}

bool ConsoleWidget::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
  if (AOrder == XSHO_CONSOLE)
  {
    showElement(AXmppStream,AStanza.element(),true);
  }
  return false;
}

void ConsoleWidget::colorXml(QString &AXml) const
{
  static const struct { const char *regexp ; const char *replace; bool minimal;} changes[] = 
  {
    { "\n",                           "<br>"                                          ,                     false      },
    { "&lt;([\\w:-]+)((\\s|/|&gt))",  "&lt;<span style='color:navy;'>\\1</span>\\2"   ,                     false      },   //open tagName
    { "&lt;/([\\w:-]+)&gt;",          "&lt;/<span style='color:navy;'>\\1</span>&gt;" ,                     false      },   //close tagName
    { " xmlns\\s?=\\s?\"(.+)\"",      " <u><span style='color:darkred;'>xmlns</span>=\"<i>\\1</i>\"</u>",   true       },   //xmlns
    { " ([\\w:-]+\\s?)=\\s?\"",       " <span style='color:darkred;'>\\1</span>=\"",                        false      }    //attribute
  }; 
  static const int changesCount = sizeof(changes)/sizeof(changes[0]);

  AXml = "<pre>"+Qt::escape(AXml)+"</pre>";
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
  Jid streamJid = ui.cmbStreamJid->currentIndex() > 0 ? ui.cmbStreamJid->itemText(ui.cmbStreamJid->currentIndex()) : "";
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
      QString caption = (ASended ? sended : received).arg(AXmppStream->streamJid().hFull()).arg(FTimePoint.toString()).arg(delta);
      ui.tedConsole->append(caption);

      QString xml = stanza.toString(2);
      hidePasswords(xml);
      if (ui.chbColoredXML->checkState() == Qt::Checked)
        colorXml(xml);
      else if (ui.chbColoredXML->checkState()==Qt::PartiallyChecked && xml.size() < 5000)
        colorXml(xml);
      else
        xml = "<pre>"+Qt::escape(xml).replace('\n',"<br>")+"</pre>";
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
      ui.tedConsole->append(tr("<b>Start sending user stanza...</b><br>"));
      foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
        if (ui.cmbStreamJid->currentIndex()==0 || stream->streamJid()==ui.cmbStreamJid->currentText())
          stream->sendStanza(stanza);
      ui.tedConsole->append(tr("<b>User stanza sended.</b><br>"));
    }
    else
    {
      ui.tedConsole->append(tr("<b>Stanza is not well formed.</b><br>"));
    }
  }
  else
  {
    ui.tedConsole->append(tr("<b>XML is not well formed.</b><br>"));
  }
}

void ConsoleWidget::onLoadContextClicked()
{
  if (FSettingsPlugin)
  {
    QString ns = ui.cmbContext->currentText();
    ISettings *settings = FSettingsPlugin->settingsForPlugin(CONSOLE_UUID);
    if (settings->isValueNSExists(SVN_CONTEXT,ns))
    {
      QString streamJid = settings->valueNS(SVN_CONTEXT_STREAM,ns).toString();
      if (streamJid.isEmpty())
        ui.cmbStreamJid->setCurrentIndex(0);
      else
        ui.cmbStreamJid->setCurrentIndex(ui.cmbStreamJid->findText(streamJid));
      
      QStringList conditions = settings->valueNS(SVN_CONTEXT_CONDITIONS,ns).toStringList();
      ui.ltwConditions->clear();
      ui.ltwConditions->addItems(conditions);

      ui.chbColoredXML->setCheckState((Qt::CheckState)settings->valueNS(SVN_CONTEXT_COLORXML,ns,Qt::PartiallyChecked).toInt());

      ui.tedSendXML->setPlainText(QString::fromUtf8(settings->loadBinaryData(QString(SBD_CONTEXT_SENDXML).arg(ns))));
      this->restoreGeometry(settings->loadBinaryData(QString(SBD_CONTEXT_GEOMETRY).arg(ns)));
      ui.sptHSplitter->restoreState(settings->loadBinaryData(QString(SBD_CONTEXT_HSPLITTER).arg(ns)));
      ui.sptVSplitter->restoreState(settings->loadBinaryData(QString(SBD_CONTEXT_VSPLITTER).arg(ns)));
    }
  }
  setWindowTitle(tr("XML Console - %1").arg(!ui.cmbContext->currentText().isEmpty() ? ui.cmbContext->currentText() : tr("Default")));
}

void ConsoleWidget::onSaveContextClicked()
{
  if (FSettingsPlugin)
  {
    QString ns = ui.cmbContext->currentText();
    ISettings *settings = FSettingsPlugin->settingsForPlugin(CONSOLE_UUID);
    QString streamJid = ui.cmbStreamJid->currentIndex()>0 ? ui.cmbStreamJid->currentText() : "";
    settings->setValueNS(SVN_CONTEXT_STREAM,ns,streamJid);

    QStringList conditions;
    for (int i=0; i<ui.ltwConditions->count(); i++)
      conditions.append(ui.ltwConditions->item(i)->text());
    settings->setValueNS(SVN_CONTEXT_CONDITIONS,ns,conditions);
    
    settings->setValueNS(SVN_CONTEXT_COLORXML,ns,ui.chbColoredXML->checkState());

    settings->saveBinaryData(QString(SBD_CONTEXT_SENDXML).arg(ns),ui.tedSendXML->toPlainText().toUtf8());
    settings->saveBinaryData(QString(SBD_CONTEXT_GEOMETRY).arg(ns),this->saveGeometry());
    settings->saveBinaryData(QString(SBD_CONTEXT_HSPLITTER).arg(ns),ui.sptHSplitter->saveState());
    settings->saveBinaryData(QString(SBD_CONTEXT_VSPLITTER).arg(ns),ui.sptVSplitter->saveState());

    if (!ns.isEmpty() && ui.cmbContext->findText(ns)<0)
      ui.cmbContext->addItem(ns);
  }
}

void ConsoleWidget::onDeleteContextClicked()
{
  if (FSettingsPlugin && !ui.cmbContext->currentText().isEmpty())
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(CONSOLE_UUID);
    settings->deleteValueNS(SVN_CONTEXT,ui.cmbContext->currentText());
    ui.cmbContext->removeItem(ui.cmbContext->findText(ui.cmbContext->currentText()));
  }
}

void ConsoleWidget::onWordWrapStateChanged( int AState )
{
  ui.tedConsole->setLineWrapMode(AState == Qt::Checked ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void ConsoleWidget::onStreamCreated(IXmppStream *AXmppStream)
{
  ui.cmbStreamJid->addItem(AXmppStream->streamJid().full());
  AXmppStream->insertXmppStanzaHandler(this, XSHO_CONSOLE);
}

void ConsoleWidget::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  int index = ui.cmbStreamJid->findText(ABefour.full());
  if (index >= 0)
  {
    ui.cmbStreamJid->removeItem(index);
    ui.cmbStreamJid->addItem(AXmppStream->streamJid().full());
  }
}

void ConsoleWidget::onStreamDestroyed(IXmppStream *AXmppStream)
{
  ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findText(AXmppStream->streamJid().full()));
  AXmppStream->removeXmppStanzaHandler(this, XSHO_CONSOLE);
}

void ConsoleWidget::onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle)
{
  Q_UNUSED(AHandleId);
  foreach(QString condition, AHandle.conditions)
    if (ui.cmbCondition->findText(condition) < 0)
      ui.cmbCondition->addItem(condition);
}

void ConsoleWidget::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(CONSOLE_UUID);
  QList<QString> contexts = settings->values(SVN_CONTEXT).keys();
  foreach(QString context, contexts)
    if (!context.isEmpty())
      ui.cmbContext->addItem(context);
  ui.cmbContext->clearEditText();
}

void ConsoleWidget::onSettingsClosed()
{
  ui.cmbContext->clear();
}
