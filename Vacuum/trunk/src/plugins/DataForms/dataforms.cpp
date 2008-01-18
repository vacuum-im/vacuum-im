#include "dataforms.h"

DataForms::DataForms()
{

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

bool DataForms::startPlugin()
{
  //QDomDocument doc;
  //QFile formFile("form.xml");
  //formFile.open(QFile::ReadOnly);
  //doc.setContent(formFile.readAll(),true);
  //formFile.close();
  //IDataDialog *dialog = newDataDialog(doc.documentElement().firstChildElement().firstChildElement("x"),NULL);
  //if (dialog)
  //  dialog->show();
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

Q_EXPORT_PLUGIN2(DataFormsPlugin, DataForms)
