#include <QtDebug>
#include "messenger.h"

Messenger::Messenger()
{
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FRostersModelPlugin = NULL;
  FTrayManager = NULL;

  FNormalLabelId = 0;
  FIndexClickHooker = 0;
  FSystemIconset.openFile(SYSTEM_ICONSETFILE);
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
  AInitOrder = MESSENGER_INITORDER;

  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
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
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

  return true;
}

bool Messenger::initObjects()
{
  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersModel = FRostersModelPlugin->rostersModel();
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FNormalLabelId = FRostersView->createIndexLabel(MESSAGE_LABEL_ORDER,FSystemIconset.iconByName("psi/sendMessage"),IRostersView::LabelBlink);
    FIndexClickHooker = FRostersView->createClickHooker(this,0);
  }

  return true;
}

bool Messenger::readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  Message message(AStanza,this);
  if (!message.body().isEmpty())
  {
    emit messageReceived(message);
    notifyMessage(message.to(),message.from(),message.type());
    //MessageWindow *mWnd = new MessageWindow(NULL);
    //emit messageWindowCreated(mWnd);
    //mWnd->setStreamJid(AStreamJid);
    //mWnd->setContactJid(message.from());
    //mWnd->dateTimeEdit()->setText(message.dateTime().toString(Qt::LocaleDate));
    //mWnd->messageEdit()->setPlainText(message.body());
    //mWnd->messageEdit()->setReadOnly(true);
    //mWnd->show();
    AAccept = true;
  }
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

IRosterIndexList Messenger::getContactIndexList(const Jid &AStreamJid, const Jid &AJid)
{
  IRosterIndexList indexList;
  if (FRostersModel)
  {
    IRosterIndex *streamRoot = FRostersModel->getStreamRoot(AStreamJid);
    if (streamRoot)
    {
      IRosterIndex::ItemType type = IRosterIndex::IT_Contact;
      if (AJid.node().isEmpty())
        type = IRosterIndex::IT_Agent;
      QHash<int,QVariant> data;
      data.insert(IRosterIndex::DR_Type, type);
      data.insert(IRosterIndex::DR_PJid, AJid.pFull());
      indexList = streamRoot->findChild(data,true);
      if (indexList.isEmpty())
      {
        data.insert(IRosterIndex::DR_PJid, AJid.pBare());
        indexList = streamRoot->findChild(data,true);
      }
      if (indexList.isEmpty())
      {
        IRoster *roster = FRostersModel->getRoster(AStreamJid.pFull());
        IRosterIndex *notInRosterGroup = FRostersModel->createGroup(FRostersModel->notInRosterGroupName(),
          roster->groupDelimiter(),IRosterIndex::IT_NotInRosterGroup,streamRoot);
        IRosterIndex *index = FRostersModel->createRosterIndex(type,AJid.pBare(),notInRosterGroup);
        index->setData(IRosterIndex::DR_Jid,AJid.full());
        index->setData(IRosterIndex::DR_PJid,AJid.pFull());
        index->setData(IRosterIndex::DR_BareJid,AJid.pBare());
        index->setData(IRosterIndex::DR_Group,FRostersModel->notInRosterGroupName());
        FRostersModel->insertRosterIndex(index,notInRosterGroup);
        indexList.append(index);
      }
    }
  }
  return indexList;
}

void Messenger::notifyMessage(const Jid &AStreamJid, const Jid &AFromJid, const QString &/*AMesType*/)
{
  if (FRostersView)
  {
    IRosterIndexList indexList = getContactIndexList(AStreamJid,AFromJid);
    foreach(IRosterIndex *index, indexList)
      FRostersView->insertIndexLabel(FNormalLabelId,index);
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

void Messenger::onStreamRemoved(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FMessageHandlers.contains(AXmppStream))
  {
    HandlerId handler = FMessageHandlers.value(AXmppStream);
    FStanzaProcessor->removeHandler(handler);
    FMessageHandlers.remove(AXmppStream);
  }
}

void Messenger::onSkinChaged()
{
  if (FRostersView)
  {
    FRostersView->updateIndexLabel(FNormalLabelId,FSystemIconset.iconByName("psi/sendMessage"),IRostersView::LabelBlink);
  }
}

Q_EXPORT_PLUGIN2(MessengerPlugin, Messenger)
