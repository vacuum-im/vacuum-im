#ifndef IDATAFORMS_H
#define IDATAFORMS_H

#include <QDomElement>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include "../../utils/toolbarchanger.h"

#define DATAFORMS_UUID "{2B8F89D0-EAA7-46eb-B2FD-AE30DF60E440}"

#define FIELD_BOOLEAN               "boolean"
#define FIELD_FIXED                 "fixed"
#define FIELD_HIDDEN                "hidden"
#define FIELD_JIDMULTI              "jid-multi"
#define FIELD_JIDSINGLE             "jid-single"
#define FIELD_LISTMULTI             "list-multi"
#define FIELD_LISTSINGLE            "list-single"
#define FIELD_TEXTMULTI             "text-multi"
#define FIELD_TEXTPRIVATE           "text-private"
#define FIELD_TEXTSINGLE            "text-single"

#define FORM_FORM                   "form"
#define FORM_SUBMIT                 "submit"
#define FORM_CANCEL                 "cancel"
#define FORM_RESULT                 "result"

class IDataField
{
public:
  enum FieldKind {
    Edit,
    View,
    Value,
    TableHeader,
    TableCell
  };
public:
  virtual QObject *instance() =0;
  virtual int kind() const =0;
  virtual QWidget *widget() const =0;
  virtual QTableWidgetItem *tableItem() const =0;
  virtual bool isValid() const =0;
  virtual bool isEmpty() const =0;
  virtual bool isRequired() const =0;
  virtual QString var() const =0;
  virtual QString type() const =0;
  virtual QString label() const =0;
  virtual QString description() const = 0;
  virtual QStringList optionLabels() const =0;
  virtual QString optionValue(const QString &ALabel) const =0;
  virtual QVariant value() const =0;
  virtual void setValue(const QVariant &AValue) =0;
signals:
  virtual void fieldGotFocus(Qt::FocusReason AReason) =0;
  virtual void fieldLostFocus(Qt::FocusReason AReason) =0;
};

class IDataForm
{
public:
  virtual QWidget *instance() =0;
  virtual bool isValid(int APage = -1) const =0;
  virtual QString invalidMessage(int APage = -1) const =0;
  virtual QDomElement formElement() const =0;
  virtual void createSubmit(QDomElement &AFormElem) const =0;
  virtual QString type() const =0;
  virtual QString title() const =0;
  virtual QStringList instructions() const =0;
  virtual void setInstructions(const QString &AInstructions) =0;
  virtual QWidget *pageControl() const =0;
  virtual int pageCount() const =0;
  virtual QString pageLabel(int APage) const =0;
  virtual int currentPage() const =0;
  virtual void setCurrentPage(int APage) =0;
  virtual QTableWidget *tableWidget() const =0;
  virtual int tableRowCount() const =0;
  virtual int tableColumnCount() const =0;
  virtual QList<IDataField *> tableHeaders() const =0;
  virtual IDataField *tableField(int ATableCol, int ATableRow) const =0;
  virtual IDataField *tableField(const QString &AVar, int ATableRow) const =0;
  virtual QList<IDataField *> fields() const =0;
  virtual IDataField *field(const QString &AVar) const =0;
  virtual IDataField *focusedField() const =0;
  virtual QVariant fieldValue(const QString &AVar) const =0;
  virtual QList<IDataField *> notValidFields() const =0;
signals:
  virtual void fieldActivated(IDataField *AField) =0;
  virtual void focusedFieldChanged(IDataField *AFocusedField) =0;
  virtual void fieldGotFocus(IDataField *AField, Qt::FocusReason AReason) =0;
  virtual void fieldLostFocus(IDataField *AField, Qt::FocusReason AReason) =0;
  virtual void currentPageChanged(int APage) =0;
};

class IDataDialog :
  public QDialog
{
public:
  IDataDialog(QWidget *AParent = NULL) : QDialog(AParent) {}
public:
  virtual IDataForm *dataForm() const =0;
  virtual ToolBarChanger *toolBarChanged() const =0;
  virtual QDialogButtonBox *dialogButtons() const =0;
  virtual void showPage(int APage) =0;
  virtual void showPrevPage() =0;
  virtual void showNextPage() =0;
  virtual void setAutoAccept(bool AAuto) =0;
};

class IDataForms
{
public:
  virtual QObject *instance() =0;
  virtual IDataForm *newDataForm(const QDomElement &AFormElement, QWidget *AParent = NULL) =0;
  virtual IDataDialog * newDataDialog(const QDomElement &AFormElement, QWidget *AParent = NULL) =0;
signals:
  virtual void dataFormCreated(IDataForm *AForm) =0;
  virtual void dataDialogCreated(IDataDialog *ADialog) =0;
};

Q_DECLARE_INTERFACE(IDataField,"Vacuum.Plugin.IDataField/1.0")
Q_DECLARE_INTERFACE(IDataForm,"Vacuum.Plugin.IDataForm/1.0")
Q_DECLARE_INTERFACE(IDataDialog,"Vacuum.Plugin.IDataDialog/1.0")
Q_DECLARE_INTERFACE(IDataForms,"Vacuum.Plugin.IDataForms/1.0")

#endif