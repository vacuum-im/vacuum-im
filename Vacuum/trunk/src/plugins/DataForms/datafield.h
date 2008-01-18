#ifndef DATAFIELD_H
#define DATAFIELD_H

#include <QUrl>
#include <QDomElement>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QDateTimeEdit>
#include <QRegExp>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>
#include <QObjectCleanupHandler>
#include "../../interfaces/idataforms.h"
#include "../../utils/jid.h"

#define VALIDATE_TYPE_STRING        "xs:string"
#define VALIDATE_TYPE_URI           "xs:anyURI"
#define VALIDATE_TYPE_DECIMAL       "xs:decimal"
#define VALIDATE_TYPE_BYTE          "xs:byte"
#define VALIDATE_TYPE_SHORT         "xs:short"
#define VALIDATE_TYPE_INT           "xs:int"
#define VALIDATE_TYPE_INTEGER       "xs:integer"
#define VALIDATE_TYPE_LONG          "xs:long"
#define VALIDATE_TYPE_DOUBLE        "xs:double"
#define VALIDATE_TYPE_DATETIME      "xs:dateTime"
#define VALIDATE_TYPE_DATE          "xs:date"
#define VALIDATE_TYPE_TIME          "xs:time"
#define VALIDATE_TYPE_LANGUAGE      "xs:language"

#define VALIDATE_METHOD_BASIC       "basic"
#define VALIDATE_METHOD_OPEN        "open"
#define VALIDATE_METHOD_RANGE       "range"
#define VALIDATE_METHOD_LIST_RANGE  "list-range"
#define VALIDATE_METHOD_REGEXP      "regex"

class DataField : 
  public QObject,
  public IDataField
{
  Q_OBJECT;
  Q_INTERFACES(IDataField);
public:
  DataField(const QDomElement &AElem, IDataField::FieldKind AKind, QWidget *AParent);
  ~DataField();
  //IDataField
  virtual QObject *instance() { return this; }
  virtual int kind() const { return FKind; }
  virtual QWidget *widget() const { return FWidget; }
  virtual QTableWidgetItem *tableItem() const { return FTableItem; } 
  virtual bool isValid() const;
  virtual bool isEmpty() const;
  virtual bool isRequired() const {return FRequired; }
  virtual QString var() const { return FVar; }
  virtual QString type() const { return FType; }
  virtual QString label() const {return FLabel; }
  virtual QString description() const { return FDesc; }
  virtual QStringList optionLabels() const { return FOptionLabels; }
  virtual QString optionValue(const QString &ALabel) const { return FOptions.value(ALabel); }
  virtual QVariant value() const;
  virtual void setValue(const QVariant &AValue);
signals:
  virtual void fieldGotFocus(Qt::FocusReason AReason);
  virtual void fieldLostFocus(Qt::FocusReason AReason);
protected:
  virtual QValidator *createValidator(QWidget *AParent);
  virtual QWidget *createValidatedEditor(QWidget *AParent);
protected:
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
private:
  QCheckBox *FCheckBox;
  QLabel *FLabelWidget;
  QTextEdit *FTextEdit;
  QGroupBox *FGroupBox;
  QList<QCheckBox *> FCheckList;
  QComboBox *FComboBox;
  QLineEdit *FLineEdit;
  QDateEdit *FDateEdit;
  QTimeEdit *FTimeEdit;
  QDateTimeEdit *FDateTimeEdit;
  QTableWidgetItem *FTableItem;
  QObjectCleanupHandler FCleanupHandler;
private:
  struct Validate 
  {
    QString dataType;
    QString method;
    QString min;
    QString max;
    QRegExp regexp;
  } 
  FValidate;
private:
  int FKind;
  bool FRequired;
  QWidget *FWidget;
  QString FVar;
  QString FType;
  QString FLabel;
  QString FDesc;
  QVariant FStaticValue;
  QList<QString> FOptionLabels;
  QHash<QString, QString> FOptions;
};

#endif // DATAFIELD_H
