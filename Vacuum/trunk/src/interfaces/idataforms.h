#ifndef IDATAFORMS_H
#define IDATAFORMS_H

#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include "../../utils/toolbarchanger.h"

#define DATAFORMS_UUID                  "{2B8F89D0-EAA7-46eb-B2FD-AE30DF60E440}"

#define DATAVALIDATE_TYPE_STRING        "xs:string"
#define DATAVALIDATE_TYPE_URI           "xs:anyURI"
#define DATAVALIDATE_TYPE_DECIMAL       "xs:decimal"
#define DATAVALIDATE_TYPE_BYTE          "xs:byte"
#define DATAVALIDATE_TYPE_SHORT         "xs:short"
#define DATAVALIDATE_TYPE_INT           "xs:int"
#define DATAVALIDATE_TYPE_INTEGER       "xs:integer"
#define DATAVALIDATE_TYPE_LONG          "xs:long"
#define DATAVALIDATE_TYPE_DOUBLE        "xs:double"
#define DATAVALIDATE_TYPE_DATETIME      "xs:dateTime"
#define DATAVALIDATE_TYPE_DATE          "xs:date"
#define DATAVALIDATE_TYPE_TIME          "xs:time"
#define DATAVALIDATE_TYPE_LANGUAGE      "xs:language"

#define DATAVALIDATE_METHOD_BASIC       "basic"
#define DATAVALIDATE_METHOD_OPEN        "open"
#define DATAVALIDATE_METHOD_RANGE       "range"
#define DATAVALIDATE_METHOD_LIST_RANGE  "list-range"
#define DATAVALIDATE_METHOD_REGEXP      "regex"

#define DATAFIELD_TYPE_BOOLEAN          "boolean"
#define DATAFIELD_TYPE_FIXED            "fixed"
#define DATAFIELD_TYPE_HIDDEN           "hidden"
#define DATAFIELD_TYPE_JIDMULTI         "jid-multi"
#define DATAFIELD_TYPE_JIDSINGLE        "jid-single"
#define DATAFIELD_TYPE_LISTMULTI        "list-multi"
#define DATAFIELD_TYPE_LISTSINGLE       "list-single"
#define DATAFIELD_TYPE_TEXTMULTI        "text-multi"
#define DATAFIELD_TYPE_TEXTPRIVATE      "text-private"
#define DATAFIELD_TYPE_TEXTSINGLE       "text-single"

#define DATAFORM_TYPE_FORM              "form"
#define DATAFORM_TYPE_SUBMIT            "submit"
#define DATAFORM_TYPE_CANCEL            "cancel"
#define DATAFORM_TYPE_RESULT            "result"

struct IDataValidate {
  QString type;
  QString method;
  QString min;
  QString max;
  QRegExp regexp;
  QString listMin;
  QString listMax;
};

struct IDataOption {
  QString label;
  QString value;
};

struct IDataField {
  bool required;
  QString var;
  QString type;
  QString label;
  QString desc;
  QVariant value;
  QList<IDataOption> options;
  IDataValidate validate;
};

struct IDataTable {
  QList<IDataField> columns;
  QMap<int,QStringList> rows;
};

struct IDataLayout {
  QString label;
  QStringList text;
  QStringList fieldrefs;
  QList<IDataLayout> sections;
  QStringList childOrder;
};

struct IDataForm {
  QString type;
  QString title;
  QStringList instructions;
  IDataTable tabel;
  QList<IDataField> fields;
  QList<IDataLayout> pages;
};

class IDataTableWidget
{
public:
  virtual QTableWidget *instance() =0;
  virtual const IDataTable &dataTable() const =0;
  virtual IDataField currentField() const =0;
  virtual IDataField dataField(int ARow, int AColumn) const =0;
  virtual IDataField dataField(int ARow, const QString &AVar) const =0;
signals:
  virtual void activated(int ARow, int AColumn) =0;
  virtual void changed(int ACurrentRow, int ACurrentColumn, int APreviousRow, int APreviousColumn) =0;
};

class IDataFieldWidget
{
public:
  virtual QWidget *instance() =0;
  virtual bool isReadOnly() const =0;
  virtual IDataField userDataField() const =0;
  virtual const IDataField &dataField() const =0;
  virtual QVariant value() const =0;
  virtual void setValue(const QVariant &AValue) =0;
signals:
  virtual void focusIn(Qt::FocusReason AReason) =0;
  virtual void focusOut(Qt::FocusReason AReason) =0;
};

class IDataFormWidget
{
public:
  virtual QWidget *instance() =0;
  virtual bool checkForm(bool AAllowInvalid) const =0;
  virtual IDataTableWidget *tableWidget() const =0;
  virtual IDataFieldWidget *fieldWidget(int AIndex) const =0;
  virtual IDataFieldWidget *fieldWidget(const QString &AVar) const =0;
  virtual IDataForm userDataForm() const =0;
  virtual const IDataForm &dataForm() const =0;
signals:
  virtual void cellActivated(int ARow, int AColumn) =0;
  virtual void cellChanged(int ACurrentRow, int ACurrentColumn, int APreviousRow, int APreviousColumn) =0;
  virtual void fieldFocusIn(IDataFieldWidget *AField, Qt::FocusReason AReason) =0;
  virtual void fieldFocusOut(IDataFieldWidget *AField, Qt::FocusReason AReason) =0;
};

class IDataDialogWidget
{
public:
  virtual QDialog *instance() =0;
  virtual ToolBarChanger *toolBarChanged() const =0;
  virtual QDialogButtonBox *dialogButtons() const =0;
  virtual IDataFormWidget *formWidget() const =0;
  virtual void setForm(const IDataForm &AForm) =0;
  virtual bool allowInvalid() const =0;
  virtual void setAllowInvalid(bool AAllowInvalid) =0;
signals:
  virtual void formWidgetCreated(IDataFormWidget *AForm) =0;
  virtual void formWidgetDestroyed(IDataFormWidget *AForm) =0;
  virtual void dialogDestroyed(IDataDialogWidget *ADialog) =0;
};

class IDataForms
{
public:
  enum FieldWriteMode {
    FWM_FORM,
    FWM_SUBMIT,
    FWM_RESULT,
    FWM_TABLE
  };
public:
  virtual QObject *instance() =0;
  //XML2DATA
  virtual IDataValidate dataValidate(const QDomElement &AValidateElem) const =0;
  virtual IDataField dataField(const QDomElement &AFieldElem) const =0;
  virtual IDataTable dataTable(const QDomElement &ATableElem) const =0;
  virtual IDataLayout dataLayout(const QDomElement &ALayoutElem) const =0;
  virtual IDataForm dataForm(const QDomElement &AFormElem) const =0;
  //DATA2XML
  virtual void xmlValidate(const IDataValidate &AValidate, QDomElement &AFieldElem) const =0;
  virtual void xmlField(const IDataField &AField, QDomElement &AFormElem, FieldWriteMode AMode = FWM_FORM) const =0;
  virtual void xmlTable(const IDataTable &ATable, QDomElement &AFormElem) const =0;
  virtual void xmlSection(const IDataLayout &ALayout, QDomElement &AParentElem) const =0;
  virtual void xmlPage(const IDataLayout &ALayout, QDomElement &AParentElem) const =0;
  virtual void xmlForm(const IDataForm &AForm, QDomElement &AParentElem) const =0;
  //Data Checks
  virtual bool isDataValid(const IDataValidate &AValidate, const QString &AValue) const =0;
  virtual bool isOptionValid(const QList<IDataOption> &AOptions, const QString &AValue) const =0;
  virtual bool isFieldEmpty(const IDataField &AField) const =0;
  virtual bool isFieldValid(const IDataField &AField) const =0;
  virtual bool isFormValid(const IDataForm &AForm) const =0;
  virtual bool isSubmitValid(const IDataForm &AForm, const IDataForm &ASubmit) const =0;
  //Data actions
  virtual int fieldIndex(const QString &AVar, const QList<IDataField> &AFields) const =0;
  virtual QVariant fieldValue(const QString &AVar, const QList<IDataField> &AFields) const =0;
  virtual IDataForm dataSubmit(const IDataForm &AForm) const =0;
  //Data widgets
  virtual QValidator *dataValidator(const IDataValidate &AValidate, QObject *AParent) const =0;
  virtual IDataTableWidget *tableWidget(const IDataTable &ATable, QWidget *AParent) =0;
  virtual IDataFieldWidget *fieldWidget(const IDataField &AField, bool AReadOnly, QWidget *AParent) =0;
  virtual IDataFormWidget *formWidget(const IDataForm &AForm, QWidget *AParent) =0;
  virtual IDataDialogWidget *dialogWidget(const IDataForm &AForm, QWidget *AParent) =0;
signals:
  virtual void tableWidgetCreated(IDataTableWidget *ATable) =0;
  virtual void fieldWidgetCreated(IDataFieldWidget *AField) =0;
  virtual void formWidgetCreated(IDataFormWidget *AForm) =0;
  virtual void dialogWidgetCreated(IDataDialogWidget *ADialog) =0;
};

Q_DECLARE_INTERFACE(IDataTableWidget,"Vacuum.Plugin.IDataTableWidget/1.0")
Q_DECLARE_INTERFACE(IDataFieldWidget,"Vacuum.Plugin.IDataFieldWidget/1.0")
Q_DECLARE_INTERFACE(IDataFormWidget,"Vacuum.Plugin.IDataFormWidget/1.0")
Q_DECLARE_INTERFACE(IDataDialogWidget,"Vacuum.Plugin.IDataDialogWidget/1.0")
Q_DECLARE_INTERFACE(IDataForms,"Vacuum.Plugin.IDataForms/1.0")

#endif
