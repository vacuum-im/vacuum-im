#ifndef DATAFORMS_H
#define DATAFORMS_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/idataforms.h"
#include "dataform.h"
#include "datadialog.h"

class DataForms : 
  public QObject,
  public IPlugin,
  public IDataForms
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IDataForms);
public:
  DataForms();
  ~DataForms();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return DATAFORMS_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager * /*APluginManager*/, int &/*AInitOrder*/) { return true; }
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //IDataForms
  virtual IDataForm *newDataForm(const QDomElement &AFormElement, QWidget *AParent = NULL);
  virtual IDataDialog * newDataDialog(const QDomElement &AFormElement, QWidget *AParent = NULL);
signals:
  virtual void dataFormCreated(IDataForm *AForm);
  virtual void dataDialogCreated(IDataDialog *ADialog);
};

#endif // DATAFORMS_H
