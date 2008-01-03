#include "vcardplugin.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDomDocument>

#define VCARD_DIRNAME             "vcards"
#define VCARD_TIMEOUT             60000
#define ADR_StreamJid             Action::DR_StreamJid
#define ADR_ContactJid            Action::DR_Parametr1

#define IN_VCARD                  "psi/vCard"

VCardPlugin::VCardPlugin()
{
  FXmppStreams = NULL;
  FRostersView = NULL; 
  FRostersViewPlugin = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;
}

VCardPlugin::~VCardPlugin()
{

}

void VCardPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing vcards");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("VCard manager"); 
  APluginInfo->uid = VCARD_UUID;
  APluginInfo->version = "0.1";
}

bool VCardPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_VCARD;
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onXmppStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  return true;
}

bool VCardPlugin::initObjects()
{
  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  return true;
}

bool VCardPlugin::startPlugin()
{
  return true;
}

void VCardPlugin::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FVCardRequestId.contains(AStanza.id()))
  {
    Jid fromJid = FVCardRequestId.take(AStanza.id());
    QDomElement elem = AStanza.firstElement(VCARD_TAGNAME,NS_VCARD_TEMP);
    if (!elem.isNull() && AStanza.type()=="result")
    {
      saveVCardFile(elem,fromJid); 
      emit vcardReceived(fromJid);
    }
    else if (AStanza.type()=="error")
    {
      ErrorHandler err(AStanza.element());
      emit vcardError(fromJid,err.message());
    }
  }
  else if (FVCardPublishId.contains(AStanza.id()))
  {
    Jid fromJid = FVCardPublishId.take(AStanza.id());
    if (AStanza.type() == "result")
    {
      VCardItem vcardItem = FVCards.value(fromJid.pBare());
      if (vcardItem.vcard)
        saveVCardFile(vcardItem.vcard->vcardElem(),fromJid);
      emit vcardPublished(fromJid);
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      emit vcardError(fromJid,err.message());
    }
  }
}

void VCardPlugin::iqStanzaTimeOut(const QString &AId)
{
  if (FVCardRequestId.contains(AId))
  {
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    emit vcardError(FVCardRequestId.take(AId),err.message());
  }
  else if (FVCardPublishId.contains(AId))
  {
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    emit vcardError(FVCardPublishId.take(AId),err.message());
  }
}

QString VCardPlugin::vcardFileName(const Jid &AContactJid) const
{
  QString fileName = Jid::encode(AContactJid.bare()).toLower()+".xml";
  QDir dir;
  if (FSettingsPlugin)
    dir.setPath(FSettingsPlugin->homeDir().path());
  if (!dir.exists(VCARD_DIRNAME))
    dir.mkdir(VCARD_DIRNAME);
  fileName = dir.path()+"/"VCARD_DIRNAME"/"+fileName;
  return fileName;
}

bool VCardPlugin::hasVCard(const Jid &AContactJid) const
{
  QString fileName = vcardFileName(AContactJid);
  return QFile::exists(fileName);
}

IVCard *VCardPlugin::vcard(const Jid &AContactJid)
{
  VCardItem &vcardItem = FVCards[AContactJid.pBare()];
  if (vcardItem.vcard == NULL)
    vcardItem.vcard = new VCard(AContactJid,this);
  vcardItem.locks++;
  return vcardItem.vcard;
}

bool VCardPlugin::requestVCard(const Jid &AStreamJid, const Jid &AContactJid)
{
  if (FStanzaProcessor)
  {
    if (FVCardRequestId.key(AContactJid.pBare()).isEmpty())
    {
      Stanza iq("iq");
      iq.setTo(AContactJid.eBare()).setType("get").setId(FStanzaProcessor->newId());
      iq.addElement(VCARD_TAGNAME,NS_VCARD_TEMP);
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,VCARD_TIMEOUT))
      {
        FVCardRequestId.insert(iq.id(),AContactJid.pBare());
        return true;
      };
    }
    else
      return true;
  }
  return false;
}

bool VCardPlugin::publishVCard(IVCard *AVCard, const Jid &AStreamJid)
{
  if (FStanzaProcessor && AVCard->isValid())
  {
    if (FVCardPublishId.key(AStreamJid.pBare()).isEmpty())
    {
      Stanza iq("iq");
      iq.setTo(AStreamJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
      QDomElement elem = iq.element().appendChild(AVCard->vcardElem().cloneNode(true)).toElement();
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,VCARD_TIMEOUT))
      {
        FVCardPublishId.insert(iq.id(),AStreamJid.pBare());
        return true;
      }
    }
    else
      return true;
  }
  return false;
}

void VCardPlugin::showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid)
{
  if (FVCardDialogs.contains(AContactJid.bare()))
  {
    VCardDialog *dialog = FVCardDialogs.value(AContactJid.bare());
    dialog->activateWindow();
  }
  else if (AStreamJid.isValid() && AContactJid.isValid())
  {
    VCardDialog *dialog = new VCardDialog(this,AStreamJid,AContactJid.bare());
    connect(dialog,SIGNAL(destroyed(QObject *)),SLOT(onVCardDialogDestroyed(QObject *)));
    FVCardDialogs.insert(dialog->contactJid(),dialog);
    dialog->show();
  }
}

void VCardPlugin::unlockVCard(const Jid &AContactJid)
{
  VCardItem &vcardItem = FVCards[AContactJid.pBare()];
  vcardItem.locks--;
  if (vcardItem.locks == 0)
  {
    VCard *vcardCopy = vcardItem.vcard;   //После remove vcardItem будет недействителен
    FVCards.remove(AContactJid.pBare());
    delete vcardCopy;
  }
}

void VCardPlugin::saveVCardFile(const QDomElement &AElem, const Jid &AContactJid) const
{
  if (!AElem.isNull() && AContactJid.isValid())
  {
    QDomDocument doc;
    QDomElement elem = doc.appendChild(doc.createElement(VCARD_FILE_ROOT_TAGNAME)).toElement();
    elem.setAttribute("jid",AContactJid.bare());
    elem.setAttribute("dateTime",QDateTime::currentDateTime().toString(Qt::ISODate));
    elem.appendChild(AElem.cloneNode(true));
    QFile vcardFile(vcardFileName(AContactJid));
    if (vcardFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      vcardFile.write(doc.toByteArray());
      vcardFile.close();
    }
  }
}

void VCardPlugin::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_StreamRoot || AIndex->type() == RIT_Contact || AIndex->type() == RIT_Agent)
  {
    Action *action = new Action(AMenu);
    action->setText(tr("vCard"));
    action->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
    action->setData(ADR_StreamJid,AIndex->data(RDR_StreamJid));
    action->setData(ADR_ContactJid,AIndex->data(RDR_Jid));
    AMenu->addAction(action,AG_VCARD_ROSTER,true);
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
  }
}

void VCardPlugin::onShowVCardDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid contactJid = action->data(ADR_ContactJid).toString();
    showVCardDialog(streamJid,contactJid);
  }
}

void VCardPlugin::onVCardDialogDestroyed(QObject *ADialog)
{
  VCardDialog *dialog = static_cast<VCardDialog *>(ADialog);
  Jid contactJid = FVCardDialogs.key(dialog);
  FVCardDialogs.remove(contactJid);
}

void VCardPlugin::onXmppStreamRemoved(IXmppStream *AXmppStream)
{
  foreach(VCardDialog *dialog, FVCardDialogs)
    if (dialog->streamJid() == AXmppStream->jid())
      dialog->deleteLater();
}

Q_EXPORT_PLUGIN2(VCardPlugin, VCardPlugin)
