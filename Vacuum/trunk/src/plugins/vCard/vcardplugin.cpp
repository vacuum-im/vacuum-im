#include "vcardplugin.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>

#define VCARD_DIRNAME             "vcards"
#define VCARD_TIMEOUT             60000
#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

#define IN_VCARD                  "psi/vCard"

VCardPlugin::VCardPlugin()
{
  FXmppStreams = NULL;
  FRostersView = NULL; 
  FRostersViewPlugin = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;
  FMultiUserChatPlugin = NULL;
  FDiscovery = NULL;
}

VCardPlugin::~VCardPlugin()
{

}

void VCardPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Request and publish vCards");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("VCard manager"); 
  APluginInfo->uid = VCARD_UUID;
  APluginInfo->version = "0.1";
}

bool VCardPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
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

  plugin = APluginManager->getPlugins("IMultiUserChatPlugin").value(0,NULL);
  if (plugin)
  {
    FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
    if (FMultiUserChatPlugin)
    {
      connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
        SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
    }
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  return true;
}

bool VCardPlugin::initObjects()
{
  if (FRostersViewPlugin)
  {
    FRostersView = FRostersViewPlugin->rostersView();
    connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  if (FDiscovery)
  {
    registerDiscoFeatures();
  }
  return true;
}

void VCardPlugin::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FVCardRequestId.contains(AStanza.id()))
  {
    Jid fromJid = FVCardRequestId.take(AStanza.id());
    QDomElement elem = AStanza.firstElement(VCARD_TAGNAME,NS_VCARD_TEMP);
    if (AStanza.type()=="result")
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
    Stanza stanza = FVCardPublishStanza.take(AStanza.id());
    if (AStanza.type() == "result")
    {
      saveVCardFile(stanza.element().firstChildElement(VCARD_TAGNAME),fromJid);
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
    FVCardPublishStanza.remove(AId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    emit vcardError(FVCardPublishId.take(AId),err.message());
  }
}

QString VCardPlugin::vcardFileName(const Jid &AContactJid) const
{
  QString fileName = Jid::encode(AContactJid.pFull())+".xml";
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
  VCardItem &vcardItem = FVCards[AContactJid];
  if (vcardItem.vcard == NULL)
    vcardItem.vcard = new VCard(AContactJid,this);
  vcardItem.locks++;
  return vcardItem.vcard;
}

bool VCardPlugin::requestVCard(const Jid &AStreamJid, const Jid &AContactJid)
{
  if (FStanzaProcessor)
  {
    if (FVCardRequestId.key(AContactJid).isEmpty())
    {
      Stanza request("iq");
      request.setTo(AContactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
      request.addElement(VCARD_TAGNAME,NS_VCARD_TEMP);
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,VCARD_TIMEOUT))
      {
        FVCardRequestId.insert(request.id(),AContactJid);
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
      Stanza publish("iq");
      publish.setTo(AStreamJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
      QDomElement elem = publish.element().appendChild(AVCard->vcardElem().cloneNode(true)).toElement();
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,publish,VCARD_TIMEOUT))
      {
        FVCardPublishId.insert(publish.id(),AStreamJid.pBare());
        FVCardPublishStanza.insert(publish.id(),publish);
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
  if (FVCardDialogs.contains(AContactJid))
  {
    VCardDialog *dialog = FVCardDialogs.value(AContactJid);
    dialog->activateWindow();
  }
  else if (AStreamJid.isValid() && AContactJid.isValid())
  {
    VCardDialog *dialog = new VCardDialog(this,AStreamJid,AContactJid);
    connect(dialog,SIGNAL(destroyed(QObject *)),SLOT(onVCardDialogDestroyed(QObject *)));
    FVCardDialogs.insert(AContactJid,dialog);
    dialog->show();
  }
}

void VCardPlugin::unlockVCard(const Jid &AContactJid)
{
  VCardItem &vcardItem = FVCards[AContactJid];
  vcardItem.locks--;
  if (vcardItem.locks == 0)
  {
    VCard *vcardCopy = vcardItem.vcard;   //После remove vcardItem будет недействителен
    FVCards.remove(AContactJid);
    delete vcardCopy;
  }
}

void VCardPlugin::saveVCardFile(const QDomElement &AElem, const Jid &AContactJid) const
{
  if (!AElem.isNull() && AContactJid.isValid())
  {
    QDomDocument doc;
    QDomElement elem = doc.appendChild(doc.createElement(VCARD_FILE_ROOT_TAGNAME)).toElement();
    elem.setAttribute("jid",AContactJid.full());
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

void VCardPlugin::registerDiscoFeatures()
{
  IDiscoFeature dfeature;

  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_VCARD);
  dfeature.var = NS_VCARD_TEMP;
  dfeature.name = tr("vCard");
  dfeature.actionName = tr("vCard");
  dfeature.description = tr("Request and publish contact vCard");
  FDiscovery->insertDiscoFeature(dfeature);
}

void VCardPlugin::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_StreamRoot || AIndex->type() == RIT_Contact || AIndex->type() == RIT_Agent)
  {
    Action *action = new Action(AMenu);
    action->setText(tr("vCard"));
    action->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
    action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
    action->setData(ADR_CONTACT_JID,Jid(AIndex->data(RDR_Jid).toString()).bare());
    AMenu->addAction(action,AG_VCARD_ROSTER,true);
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
  }
}

void VCardPlugin::onMultiUserContextMenu(IMultiUserChatWindow * /*AWindow*/, IMultiUser *AUser, Menu *AMenu)
{
  Action *action = new Action(AMenu);
  action->setText(tr("vCard"));
  action->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
  action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAMJID));
  if (!AUser->data(MUDR_REALJID).toString().isEmpty())
    action->setData(ADR_CONTACT_JID,Jid(AUser->data(MUDR_REALJID).toString()).bare());
  else
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_CONTACTJID));
  AMenu->addAction(action,AG_MUCM_VCARD,true);
  connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
}

void VCardPlugin::onShowVCardDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    showVCardDialog(streamJid,contactJid);
  }
}

void VCardPlugin::onVCardDialogDestroyed(QObject *ADialog)
{
  VCardDialog *dialog = static_cast<VCardDialog *>(ADialog);
  FVCardDialogs.remove(FVCardDialogs.key(dialog));
}

void VCardPlugin::onXmppStreamRemoved(IXmppStream *AXmppStream)
{
  foreach(VCardDialog *dialog, FVCardDialogs)
    if (dialog->streamJid() == AXmppStream->jid())
      dialog->deleteLater();
}

Q_EXPORT_PLUGIN2(VCardPlugin, VCardPlugin)
