#ifndef DATATABLEWIDGET_H
#define DATATABLEWIDGET_H

#include <interfaces/idataforms.h>

class DataTableWidget : 
  public QTableWidget,
  public IDataTableWidget
{
  Q_OBJECT;
  Q_INTERFACES(IDataTableWidget);
 public:
  DataTableWidget(IDataForms *ADataForms, const IDataTable &ATable, QWidget *AParent);
  ~DataTableWidget();
  virtual QTableWidget *instance() { return this; }
  virtual const IDataTable &dataTable() const { return FTable; }
  virtual IDataField currentField() const;
  virtual IDataField dataField(int ARow, int AColumn) const;
  virtual IDataField dataField(int ARow, const QString &AVar) const;
signals:
  virtual void activated(int ARow, int AColumn);
  virtual void changed(int ACurrentRow, int ACurrentColumn, int APreviousRow, int APreviousColumn);
private:
  IDataForms *FDataForms;
private:
  IDataTable FTable;    
};

#endif // DATATABLEWIDGET_H
