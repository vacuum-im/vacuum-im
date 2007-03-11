#include "rosterchanger.h"

RosterChanger::RosterChanger()
{

}

RosterChanger::~RosterChanger()
{

}

//IPlugin
void RosterChanger::getPluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Manipulating roster items and groups");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster Changer"); 
  APluginInfo->uid = ROSTERCHANGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{5306971C-2488-40d9-BA8E-C83327B2EED5}"); //IRoster  
}

bool RosterChanger::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  return FRosterPlugin!=NULL;
}

bool RosterChanger::startPlugin()
{
  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(const QModelIndex &, Menu *)),
      SLOT(onRostersViewContextMenu(const QModelIndex &, Menu *)));
  }

  return true;
}

void RosterChanger::onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
  if (!AIndex.isValid())
    return;

  int itemType = AIndex.data(IRosterIndex::DR_Type).toInt();
  if (itemType == IRosterIndex::IT_Contact)
  {
    Action *renameItem = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
    renameItem->setText(tr("Rename"));
    renameItem->setData(Action::DR_Parametr1,AIndex.data(IRosterIndex::DR_RosterJid));
    renameItem->setData(Action::DR_Parametr2,AIndex.data(IRosterIndex::DR_RosterName));
    renameItem->setData(Action::DR_StreamJid,AIndex.data(IRosterIndex::DR_StreamJid));
    connect(renameItem,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
    AMenu->addAction(renameItem);
  }
}

void RosterChanger::onRenameItem(bool)
{

}

void RosterChanger::onCopyItemToGroup(bool)
{

}

void RosterChanger::onMoveItemToGroup(bool)
{

}

void RosterChanger::onRemoveItemFromGroup(bool)
{

}

void RosterChanger::onRenameGroup(bool)
{

}

void RosterChanger::onRemoveGroup(bool)
{

}

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)
