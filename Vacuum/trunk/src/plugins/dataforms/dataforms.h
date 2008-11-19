#ifndef DATAFORMS_H
#define DATAFORMS_H

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QObjectCleanupHandler>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iservicediscovery.h"
#include "datatablewidget.h"
#include "datamediawidget.h"
#include "datafieldwidget.h"
#include "dataformwidget.h"
#include "datadialogwidget.h"

struct UrlLoadState
{
  QNetworkReply *reply;
};

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
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //XML2DATA
  virtual IDataValidate dataValidate(const QDomElement &AValidateElem) const;
  virtual IDataMedia dataMedia(const QDomElement &AMediaElem) const;
  virtual IDataField dataField(const QDomElement &AFieldElem) const;
  virtual IDataTable dataTable(const QDomElement &ATableElem) const;
  virtual IDataLayout dataLayout(const QDomElement &ALayoutElem) const;
  virtual IDataForm dataForm(const QDomElement &AFormElem) const;
  //DATA2XML
  virtual void xmlValidate(const IDataValidate &AValidate, QDomElement &AFieldElem) const;
  virtual void xmlMedia(const IDataMedia &AMedia, QDomElement &AFieldElem) const;
  virtual void xmlField(const IDataField &AField, QDomElement &AFormElem, const QString &AFormType = DATAFORM_TYPE_FORM) const;
  virtual void xmlTable(const IDataTable &ATable, QDomElement &AFormElem) const;
  virtual void xmlSection(const IDataLayout &ALayout, QDomElement &AParentElem) const;
  virtual void xmlPage(const IDataLayout &ALayout, QDomElement &AParentElem) const;
  virtual void xmlForm(const IDataForm &AForm, QDomElement &AParentElem) const;
  //Data Checks
  virtual bool isDataValid(const IDataValidate &AValidate, const QString &AValue) const;
  virtual bool isOptionValid(const QList<IDataOption> &AOptions, const QString &AValue) const;
  virtual bool isMediaValid(const IDataMedia &AMedia) const;
  virtual bool isFieldEmpty(const IDataField &AField) const;
  virtual bool isFieldValid(const IDataField &AField, const QString &AFormType = DATAFORM_TYPE_FORM) const;
  virtual bool isFormValid(const IDataForm &AForm) const;
  virtual bool isSubmitValid(const IDataForm &AForm, const IDataForm &ASubmit) const;
  virtual bool isSupportedUri(const IDataMediaURI &AUri) const;
  //Localization
  virtual IDataForm localizeForm(const IDataForm &AForm) const;
  virtual IDataLocalizer *dataLocalizer(const QString &AFormType) const;
  virtual void insertLocalizer(IDataLocalizer *ALocalizer, const QString &ATypeField);
  virtual void removeLocalizer(IDataLocalizer *ALocalizer, const QString &ATypeField = "");
  //Data actions
  virtual int fieldIndex(const QString &AVar, const QList<IDataField> &AFields) const;
  virtual QVariant fieldValue(const QString &AVar, const QList<IDataField> &AFields) const;
  virtual IDataForm dataSubmit(const IDataForm &AForm) const;
  virtual QByteArray urlData(const QUrl &AUrl) const;
  virtual bool loadUrl(const QUrl &AUrl, bool AEnableCache = true);
  //Data widgets
  virtual QValidator *dataValidator(const IDataValidate &AValidate, QObject *AParent) const;
  virtual IDataTableWidget *tableWidget(const IDataTable &ATable, QWidget *AParent);
  virtual IDataMediaWidget *mediaWidget(const IDataMedia &AMedia, QWidget *AParent);
  virtual IDataFieldWidget *fieldWidget(const IDataField &AField, bool AReadOnly, QWidget *AParent);
  virtual IDataFormWidget *formWidget(const IDataForm &AForm, QWidget *AParent);
  virtual IDataDialogWidget *dialogWidget(const IDataForm &AForm, QWidget *AParent);
signals:
  virtual void tableWidgetCreated(IDataTableWidget *ATable);
  virtual void mediaWidgetCreated(IDataMediaWidget *AMedia);
  virtual void fieldWidgetCreated(IDataFieldWidget *AField);
  virtual void formWidgetCreated(IDataFormWidget *AForm);
  virtual void dialogWidgetCreated(IDataDialogWidget *ADialog);
  virtual void urlLoaded(const QUrl &AUrl, const QByteArray &AData);
  virtual void urlLoadFailed(const QUrl &AUrl, const QString &AError);
protected:
  void xmlLayout(const IDataLayout &ALayout, QDomElement &ALayoutElem) const;
  void urlLoadSuccess(const QUrl &AUrl, const QByteArray &AData);
  void urlLoadFailure(const QUrl &AUrl, const QString &AError);
  void registerDiscoFeatures();
protected slots:
  void onNetworkReplyFinished();
  void onNetworkReplyError(QNetworkReply::NetworkError ACode);
  void onNetworkReplySSLErrors(const QList<QSslError> &AErrors);
private:
  ISettings *FSettings;
  IServiceDiscovery *FDiscovery;
private:
  QNetworkAccessManager FNetworkManager;
  QHash<QUrl,UrlLoadState> FLoadStates;
  QHash<QString, IDataLocalizer *> FLocalizers;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // DATAFORMS_H
