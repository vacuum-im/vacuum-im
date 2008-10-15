#ifndef DATAFIELDWIDGET_H
#define DATAFIELDWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QListWidget>
#include <QDateTimeEdit>
#include "../../interfaces/idataforms.h"
#include "../../utils/jid.h"

class DataFieldWidget : 
  public QWidget,
  public IDataFieldWidget
{
  Q_OBJECT;
  Q_INTERFACES(IDataFieldWidget);
public:
  DataFieldWidget(IDataForms *ADataForms, const IDataField &AField, bool AReadOnly, QWidget *AParent);
  ~DataFieldWidget();
  virtual QWidget *instance() { return this; }
  virtual bool isReadOnly() const { return FReadOnly; }
  virtual IDataField userDataField() const;
  virtual const IDataField &dataField() const { return FField; }
  virtual QVariant value() const;
  virtual void setValue(const QVariant &AValue);
  virtual IDataMediaWidget *mediaWidget() const;
signals:
  virtual void focusIn(Qt::FocusReason AReason);
  virtual void focusOut(Qt::FocusReason AReason);
protected:
  void appendLabel(const QString &AText, QWidget *ABuddy);
protected:
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
private:
  IDataForms *FDataForms;
  IDataMediaWidget *FMediaWidget;
private:
  QLabel *FLabel;
  QLineEdit *FLineEdit;
  QComboBox *FComboBox;
  QCheckBox *FCheckBox;
  QTextEdit *FTextEdit;
  QDateEdit *FDateEdit;
  QTimeEdit *FTimeEdit;
  QListWidget *FListWidget;
  QDateTimeEdit *FDateTimeEdit;
private:
  bool FReadOnly;
  IDataField FField;    
};

#endif // DATAFIELDWIDGET_H
