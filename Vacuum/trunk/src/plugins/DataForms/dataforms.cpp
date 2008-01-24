#include "dataforms.h"

DataForms::DataForms()
{
  FDiscovery = NULL;
}

DataForms::~DataForms()
{

}

void DataForms::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implements data forms and generic data description");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Data Forms";
  APluginInfo->uid = DATAFORMS_UUID;
  APluginInfo->version = "0.1";
}

bool DataForms::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }
  return true;
}

bool DataForms::initObjects()
{
  registerDiscoFeatures();
  return true;
}

IDataForm *DataForms::newDataForm(const QDomElement &AFormElement, QWidget *AParent)
{
  IDataForm *form = new DataForm(AFormElement,AParent);
  emit dataFormCreated(form);
  return form;
}

IDataDialog *DataForms::newDataDialog(const QDomElement &AFormElement, QWidget *AParent)
{
  IDataForm *form = newDataForm(AFormElement,NULL);
  IDataDialog *dialog = new DataDialog(form,AParent);
  emit dataDialogCreated(dialog);
  return dialog;
}

void DataForms::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = true;
  dfeature.icon = QIcon();
  dfeature.var = NS_JABBER_DATA;
  dfeature.name = tr("Data Forms");
  dfeature.actionName = "";
  dfeature.description = tr("Implements data forms and generic data description");
  FDiscovery->insertDiscoFeature(dfeature);
}

Q_EXPORT_PLUGIN2(DataFormsPlugin, DataForms)
