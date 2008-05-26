#ifndef DATAFORMS_H
#define DATAFORMS_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/iservicediscovery.h"
#include "datatablewidget.h"
#include "datafieldwidget.h"
#include "dataformwidget.h"
#include "datadialogwidget.h"

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
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //XML2DATA
  virtual IDataValidate dataValidate(const QDomElement &AValidateElem) const;
  virtual IDataField dataField(const QDomElement &AFieldElem) const;
  virtual IDataTable dataTable(const QDomElement &ATableElem) const;
  virtual IDataLayout dataLayout(const QDomElement &ALayoutElem) const;
  virtual IDataForm dataForm(const QDomElement &AFormElem) const;
  //DATA2XML
  virtual void xmlValidate(const IDataValidate &AValidate, QDomElement &AFieldElem) const;
  virtual void xmlField(const IDataField &AField, QDomElement &AFormElem, FieldWriteMode AMode = FWM_FORM) const;
  virtual void xmlTable(const IDataTable &ATable, QDomElement &AFormElem) const;
  virtual void xmlSection(const IDataLayout &ALayout, QDomElement &AParentElem) const;
  virtual void xmlPage(const IDataLayout &ALayout, QDomElement &AParentElem) const;
  virtual void xmlForm(const IDataForm &AForm, QDomElement &AParentElem) const;
  //Data Checks
  virtual bool isDataValid(const IDataValidate &AValidate, const QString &AValue) const;
  virtual bool isOptionValid(const QList<IDataOption> &AOptions, const QString &AValue) const;
  virtual bool isFieldEmpty(const IDataField &AField) const;
  virtual bool isFieldValid(const IDataField &AField) const;
  virtual bool isFormValid(const IDataForm &AForm) const;
  virtual bool isSubmitValid(const IDataForm &AForm, const IDataForm &ASubmit) const;
  //Data actions
  virtual int fieldIndex(const QString &AVar, const QList<IDataField> &AFields) const;
  virtual QVariant fieldValue(const QString &AVar, const QList<IDataField> &AFields) const;
  virtual IDataForm dataSubmit(const IDataForm &AForm) const;
  //Data widgets
  virtual QValidator *dataValidator(const IDataValidate &AValidate, QObject *AParent) const;
  virtual IDataTableWidget *tableWidget(const IDataTable &ATable, QWidget *AParent);
  virtual IDataFieldWidget *fieldWidget(const IDataField &AField, bool AReadOnly, QWidget *AParent);
  virtual IDataFormWidget *formWidget(const IDataForm &AForm, QWidget *AParent);
  virtual IDataDialogWidget *dialogWidget(const IDataForm &AForm, QWidget *AParent);
signals:
  virtual void tableWidgetCreated(IDataTableWidget *ATable);
  virtual void fieldWidgetCreated(IDataFieldWidget *AField);
  virtual void formWidgetCreated(IDataFormWidget *AForm);
  virtual void dialogWidgetCreated(IDataDialogWidget *ADialog);
protected:
  void xmlLayout(const IDataLayout &ALayout, QDomElement &ALayoutElem) const;
  void registerDiscoFeatures();
private:
  IServiceDiscovery *FDiscovery;
};

#endif // DATAFORMS_H
