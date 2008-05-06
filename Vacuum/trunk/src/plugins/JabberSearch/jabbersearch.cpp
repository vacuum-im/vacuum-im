#include "jabbersearch.h"

#define SEARCH_TIMEOUT          30000

#define IN_SEARCH               "psi/search"

#define ADR_StreamJid           Action::DR_StreamJid
#define ADR_ServiveJid          Action::DR_Parametr1

JabberSearch::JabberSearch()
{
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FPresencePlugin = NULL;
  FDataForms = NULL;
}

JabberSearch::~JabberSearch()
{

}

void JabberSearch::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Search information repositories on the Jabber network.");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Jabber Search"); 
  APluginInfo->uid = JABBERSEARCH_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool JabberSearch::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
  
  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}

bool JabberSearch::initObjects()
{
  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_JABBER_SEARCH,this,DFO_DEFAULT);
  }
  return true;
}

void JabberSearch::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FRequests.contains(AStanza.id()))
  {
    FRequests.removeAt(FRequests.indexOf(AStanza.id()));
    if (AStanza.type() == "result")
    {
      QDomElement query = AStanza.firstElement("query",NS_JABBER_SEARCH);

      ISearchFields fields;
      fields.serviceJid = AStanza.from();
      fields.fieldMask = 0;
      fields.instructions = query.firstChildElement("instructions").text();
      if (!query.firstChildElement("first").isNull())
      {
        fields.fieldMask += ISearchFields::First;
        fields.item.firstName = query.firstChildElement("first").text();
      }
      if (!query.firstChildElement("last").isNull())
      {
        fields.fieldMask += ISearchFields::Last;
        fields.item.lastName = query.firstChildElement("last").text();
      }
      if (!query.firstChildElement("nick").isNull())
      {
        fields.fieldMask += ISearchFields::Nick;
        fields.item.nick = query.firstChildElement("nick").text();
      }
      if (!query.firstChildElement("email").isNull())
      {
        fields.fieldMask += ISearchFields::Email;
        fields.item.email = query.firstChildElement("email").text();
      }

      if (FDataForms)
      {
        QDomElement formElem = query.firstChildElement("x");
        while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
          formElem = formElem.nextSiblingElement("x");
        if (!formElem.isNull())
          fields.form = FDataForms->dataForm(formElem);
      }
     
      emit searchFields(AStanza.id(),fields);
    }
    else
      emit searchError(AStanza.id(),ErrorHandler(AStanza.element()).message());
  }
  else if (FSubmits.contains(AStanza.id()))
  {
    FSubmits.removeAt(FRequests.indexOf(AStanza.id()));
    if (AStanza.type() == "result")
    {
      QDomElement query = AStanza.firstElement("query",NS_JABBER_SEARCH);

      ISearchResult result;
      result.serviceJid = AStanza.from();
      QDomElement itemElem = query.firstChildElement("item");
      while (!itemElem.isNull())
      {
        ISearchItem item;
        item.itemJid = itemElem.attribute("jid");
        item.firstName = itemElem.firstChildElement("first").text();
        item.lastName = itemElem.firstChildElement("last").text();
        item.nick = itemElem.firstChildElement("nick").text();
        item.email = itemElem.firstChildElement("email").text();
        result.items.append(item);
      }

      if (FDataForms)
      {
        QDomElement formElem = query.firstChildElement("x");
        while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
          formElem = formElem.nextSiblingElement("x");
        if (!formElem.isNull())
          result.form = FDataForms->dataForm(formElem);
      }

      emit searchResult(AStanza.id(),result);
    }
    else
      emit searchError(AStanza.id(),ErrorHandler(AStanza.element()).message());
  }
}

void JabberSearch::iqStanzaTimeOut(const QString &AId)
{
  if (FRequests.contains(AId))
  {
    FRequests.removeAt(FRequests.indexOf(AId));
    emit searchError(AId,ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
  }
  else if (FSubmits.contains(AId))
  {
    FSubmits.removeAt(FSubmits.indexOf(AId));
    emit searchError(AId,ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
  }
}

bool JabberSearch::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_JABBER_SEARCH)
  {
    showSearchDialog(AStreamJid,ADiscoInfo.contactJid,NULL);
    return true;
  }
  return false;
}

Action *JabberSearch::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (AFeature == NS_JABBER_SEARCH)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Search"));
    action->setIcon(SYSTEM_ICONSETFILE,IN_SEARCH);
    action->setData(ADR_StreamJid,AStreamJid.full());
    action->setData(ADR_ServiveJid,ADiscoInfo.contactJid.full());
    connect(action,SIGNAL(triggered(bool)),SLOT(onSearchActionTriggered(bool)));
    return action;
  }
  return NULL;
}

QString JabberSearch::sendRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
  Stanza request("iq");
  request.setTo(AServiceJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
  request.addElement("query",NS_JABBER_SEARCH);
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,SEARCH_TIMEOUT))
  {
    FRequests.append(request.id());
    return request.id();
  }
  return QString();
}

QString JabberSearch::sendSubmit(const Jid &AStreamJid, const ISearchSubmit &ASubmit)
{
  Stanza submit("iq");
  submit.setTo(ASubmit.serviceJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
  
  QDomElement query = submit.addElement("query",NS_JABBER_SEARCH);
  if (ASubmit.form.type.isEmpty())
  {
    if (!ASubmit.item.firstName.isEmpty())
      query.appendChild(submit.createElement("first")).appendChild(submit.createTextNode(ASubmit.item.firstName));
    if (!ASubmit.item.lastName.isEmpty())
      query.appendChild(submit.createElement("last")).appendChild(submit.createTextNode(ASubmit.item.lastName));
    if (!ASubmit.item.nick.isEmpty())
      query.appendChild(submit.createElement("nick")).appendChild(submit.createTextNode(ASubmit.item.nick));
    if (!ASubmit.item.email.isEmpty())
      query.appendChild(submit.createElement("email")).appendChild(submit.createTextNode(ASubmit.item.email));
  }
  else if (FDataForms)
    FDataForms->xmlForm(ASubmit.form,query);

  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,submit,SEARCH_TIMEOUT))
  {
    FSubmits.append(submit.id());
    return submit.id();
  }
  return QString();
}

void JabberSearch::showSearchDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    SearchDialog *dialog = new SearchDialog(this,FPluginManager,AStreamJid,AServiceJid,AParent);
    connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    dialog->show();
  }
}

void JabberSearch::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_SEARCH);
  dfeature.var = NS_JABBER_SEARCH;
  dfeature.name = tr("Jabber Search");
  dfeature.description = tr("Search information repositories on the Jabber network");
  FDiscovery->insertDiscoFeature(dfeature);
}

void JabberSearch::onSearchActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid serviceJid = action->data(ADR_ServiveJid).toString();
    showSearchDialog(streamJid,serviceJid,NULL);
  }
}

Q_EXPORT_PLUGIN2(JabberSearchPlugin, JabberSearch)
