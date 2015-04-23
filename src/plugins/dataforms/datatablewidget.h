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
	virtual QTableWidget *instance() { return this; }
	virtual IDataTable dataTable() const;
	virtual IDataField currentField() const;
	virtual IDataField dataField(int ARow, int AColumn) const;
	virtual IDataField dataField(int ARow, const QString &AVar) const;
signals:
	void activated(int ARow, int AColumn);
	void changed(int ARow, int AColumn, int APrevRow, int APrevColumn);
private:
	IDataForms *FDataForms;
private:
	IDataTable FTable;
};

#endif // DATATABLEWIDGET_H
