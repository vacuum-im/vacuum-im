#include <QtDebug>
#include "messenger.h"

#define IN_NORMAL_MESSAGE "psi/sendMessage"

Messenger::Messenger()
{
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FRostersModelPlugin = NULL;
  FTrayManager = NULL;

  FMessageId = 0;
  FIndexClickHooker = 0;
  FNormalLabelId = 0;

}

Messenger::~Messenger()
{

}

void Messenger::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Send and receive messages");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Messenger"); 
  APluginInfo->uid = MESSENGER_UUID;
  APluginInfo->version = "0.1";
}

bool Messenger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_MESSENGER;

  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }
  
  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin) 
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin) 
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin) 
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(notifyActivated(int)),SLOT(onTrayNotifyActivated(int)));
    }
  }
  return true;
}

bool Messenger::initObjects()
{
  FSystemIconset.openFile(SYSTEM_ICONSETFILE);
  FMessageIcon = FSystemIconset.iconByName(IN_NORMAL_MESSAGE);

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersModel = FRostersModelPlugin->rostersModel();
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FNormalLabelId = FRostersView->createIndexLabel(RLO_MESSAGE,FMessageIcon,IRostersView::LabelBlink);
    FIndexClickHooker = FRostersView->createClickHooker(this,0);
    connect(FRostersView,SIGNAL(labelDoubleClicked(IRosterIndex *, int, bool &)),
      SLOT(onRosterLabelDClicked(IRosterIndex *, int, bool &)));
  }

  return true;
}

bool Messenger::readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  AAccept = true;
  Message message(AStanza);
  receiveMessage(message);
  return false;
}

bool Messenger::rosterIndexClicked(IRosterIndex *AIndex, int AHookerId)
{
  if (AHookerId == FIndexClickHooker)
  {
    return true;
  }
  return false;
}

int Messenger::receiveMessage(const Message &AMessage)
{
  int mesId = newMessageId();
  
  Message message = AMessage;
  message.setData(MDR_MESSAGEID,mesId);
  FMessages.insert(mesId,message);
  
  notifyMessage(mesId);
  emit messageReceived(message);
  
  return mesId;
}

void Messenger::showMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    Message &message = FMessages[AMessageId];

    removeMessage(AMessageId);
  }
}

void Messenger::removeMessage(int AMessageId)
{
  unNotifyMessage(AMessageId);
  FMessages.remove(AMessageId);
}

QList<int> Messenger::messages(const Jid &AStreamJid, const Jid &AFromJid, const QString &AMesType)
{
  QList<int> mIds;
  QMap<int,Message>::const_iterator it = FMessages.constBegin();
  while(it != FMessages.constEnd())
  {
    if (AStreamJid == it.value().to() 
        && (!AFromJid.isValid() || AFromJid == it.value().from())
        && (    (AMesType == "all" || AMesType == it.value().type()) 
            ||  (AMesType.isEmpty() && it.value().type() == "normal")
            ||  (AMesType =="normal" && it.value().type().isEmpty()) )
       )
    {
      mIds.append(it.key());
    }
    it++;
  }
  return mIds;
}

void Messenger::notifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    Message &message = FMessages[AMessageId];

    Jid fromJid = message.from();
    
    const QString toolTipMask = tr("Message from %1%2");
    QString fromName = fromJid.bare();
    QString fromResource = fromJid.resource();

    if (FRostersView && FRostersModel)
    {
      IRosterIndexList indexList = FRostersModel->getContactIndexList(message.to(),message.from(),true);
      if (!indexList.isEmpty())
      {
        QString indexName = indexList.at(0)->data(RDR_Name).toString(); 
        if (!indexName.isEmpty())
          fromName = indexName;
        foreach(IRosterIndex *index, indexList)
          FRostersView->insertIndexLabel(FNormalLabelId,index);
      }
    }

    QString toolTip = toolTipMask.arg(fromName).arg("/"+fromResource);
    message.setData(MDR_MESSAGE_TOOLTIP,toolTip);

    if (FTrayManager)
    {
      int trayId = FTrayManager->appendNotify(FMessageIcon,toolTip,true);
      FTrayId2MessageId.insert(trayId,AMessageId);
    }
  }
}

void Messenger::unNotifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    const Message &message = FMessages.value(AMessageId);

    if (FRostersView && FRostersModel && messages(message.to(),message.from()).count() == 1)
    {
      IRosterIndexList indexList = FRostersModel->getContactIndexList(message.to(),message.from());
      foreach(IRosterIndex *index, indexList)
        FRostersView->removeIndexLabel(FNormalLabelId,index);
    }

    if (FTrayManager)
    {
      int trayId = FTrayId2MessageId.key(AMessageId);
      FTrayManager->removeNotify(trayId);
      FTrayId2MessageId.remove(trayId);
    }
  }
}

void Messenger::onStreamAdded(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && !FMessageHandlers.contains(AXmppStream))
  {
    HandlerId handler = FStanzaProcessor->setHandler(this,"/message",IStanzaProcessor::DirectionIn,0,AXmppStream->jid());
    FMessageHandlers.insert(AXmppStream,handler);
  }
}

void Messenger::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  QMap<int,Message>::iterator it = FMessages.begin();
  while (it != FMessages.end())
  {
    if (AXmppStream->jid() == it.value().to())
    {
      unNotifyMessage(it.key());
      it.value().setTo(AAfter.full());
    }
    it++;
  }
}

void Messenger::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &/*ABefour*/)
{
  QMap<int,Message>::iterator it = FMessages.begin();
  while (it != FMessages.end())
  {
    if (AXmppStream->jid() == it.value().to())
      notifyMessage(it.key());
    it++;
  }
}

void Messenger::onStreamRemoved(IXmppStream *AXmppStream)
{
  // Удалить все нотификации и закрыть все окна связанные с AXmppStream
  QList<int> mIds = messages(AXmppStream->jid());
  foreach (int mId, mIds)
    removeMessage(mId);

  if (FStanzaProcessor && FMessageHandlers.contains(AXmppStream))
  {
    HandlerId handler = FMessageHandlers.value(AXmppStream);
    FStanzaProcessor->removeHandler(handler);
    FMessageHandlers.remove(AXmppStream);
  }
}

void Messenger::onSkinChaged()
{
  FMessageIcon = FSystemIconset.iconByName(IN_NORMAL_MESSAGE);
  if (FRostersView)
    FRostersView->updateIndexLabel(FNormalLabelId,FMessageIcon,IRostersView::LabelBlink);
}

void Messenger::onRosterLabelDClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted)
{
  if (ALabelId == FNormalLabelId)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    QList<int> mIds = messages(streamJid,contactJid/*,"normal"*/);
    if (!mIds.isEmpty())
    {
      showMessage(mIds.at(0));
      AAccepted = true;
    }
  }
}

void Messenger::onTrayNotifyActivated(int ANotifyId)
{
  if (FTrayId2MessageId.contains(ANotifyId))
    showMessage(FTrayId2MessageId.value(ANotifyId));
}

Q_EXPORT_PLUGIN2(MessengerPlugin, Messenger)
