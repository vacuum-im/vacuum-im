#ifndef DATAFORM_H
#define DATAFORM_H

#include <QDomNode>
#include <QDomNodeList>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStackedLayout>
#include "../../definations/namespaces.h"
#include "../../interfaces/idataforms.h"
#include "datafield.h"

class DataForm :
  public QWidget,
  public IDataForm
{
  Q_OBJECT;
  Q_INTERFACES(IDataForm);
public:
  DataForm(const QDomElement &AElem, QWidget *AParent);
  ~DataForm();
  //IDataForm
  virtual QWidget *instance() { return this; }
  virtual bool isValid() const;
  virtual QDomElement formElement() const { return FFormDoc.documentElement().firstChildElement("x"); }
  virtual void createSubmit(QDomElement &AFormElem) const;
  virtual QString type() const { return FType; }
  virtual QString title() const { return FTitle; }
  virtual QStringList instructions() const { return FInstructions; }
  virtual int pageCount() const { return FStackedWidget->count(); }
  virtual QString pageLabel(int APage) const { return FPageLabels.value(APage); }
  virtual int currentPage() const { return FStackedWidget->currentIndex(); }
  virtual void setCurrentPage(int APage);
  virtual QTableWidget *tableWidget() const { return FTableWidget; }
  virtual int tableRowCount() const { return FRows.count(); }
  virtual int tableColumnCount() const { return FColumns.count(); }
  virtual QList<IDataField *> tableHeaders() const { return FColumns; }
  virtual IDataField *tableField(int ATableCol, int ATableRow) const;
  virtual IDataField *tableField(const QString &AVar, int ATableRow) const;
  virtual QList<IDataField *> fields() const { return FDataFields; }
  virtual IDataField *field(const QString &AVar) const;
  virtual IDataField *focusedField() const { return FFocusedField; }
  virtual QVariant fieldValue(const QString &AVar) const;
  virtual QList<IDataField *> notValidFields() const;
signals:
  virtual void fieldActivated(IDataField *AField);
  virtual void focusedFieldChanged(IDataField *AFocusedField);
  virtual void fieldGotFocus(IDataField *AField, Qt::FocusReason AReason);
  virtual void fieldLostFocus(IDataField *AField, Qt::FocusReason AReason);
protected:
  void insertFields(const QDomElement &AElem, QLayout *ALayout);
  void setFocusedField(IDataField *AField, Qt::FocusReason AReason);
protected:
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
  void onFieldGotFocus(Qt::FocusReason AReason);
  void onFieldLostFocus(Qt::FocusReason AReason);
  void onCellActivated(int ARow, int ACol);
  void onCurrentCellChanged(int ARow, int ACol, int APrevRow, int APrevCol);
private:
  QDomDocument FFormDoc;
  QScrollArea *FScrollArea;
  QStackedWidget *FStackedWidget;
  QList<QWidget *> FInsertedFields;
private:
  QString FType;
  QString FTitle;
  QStringList FInstructions;
  IDataField *FFocusedField;
  QList<IDataField *> FDataFields;
private:
  QList<IDataField *> FColumns;
  QMap<int, QHash<QString,IDataField *> > FRows;
  QTableWidget *FTableWidget;
private:
  QStringList FPageLabels;
};

#endif // DATAFORM_H
