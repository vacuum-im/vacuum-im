#include "datatablewidget.h"

#include <QHeaderView>

#define DTR_COL_INDEX         Qt::UserRole
#define DTR_ROW_INDEX         Qt::UserRole+1

DataTableWidget::DataTableWidget(IDataForms *ADataForms, const IDataTable &ATable, QWidget *AParent) : QTableWidget(AParent)
{
	FTable = ATable;
	FDataForms = ADataForms;
	setRowCount(ATable.rows.count());
	setColumnCount(ATable.columns.count());

	int row = 0;
	foreach(QStringList values, ATable.rows)
	{
		for (int col=0; col<values.count(); col++)
		{
			QTableWidgetItem *item = new QTableWidgetItem(values.at(col));
			item->setData(DTR_COL_INDEX,col);
			item->setData(DTR_ROW_INDEX,row);
			item->setFlags(Qt::ItemIsEnabled);
			setItem(row,col,item);
		}
		row++;
	}

	QStringList headers;
	foreach(IDataField field, ATable.columns)
	headers.append(!field.label.isEmpty() ? field.label : field.var);

	setHorizontalHeaderLabels(headers);
	horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	connect(this,SIGNAL(cellActivated(int,int)),SIGNAL(activated(int,int)));
	connect(this,SIGNAL(currentCellChanged(int,int,int,int)),SIGNAL(changed(int,int,int,int)));
}

DataTableWidget::~DataTableWidget()
{

}

IDataField DataTableWidget::currentField() const
{
	return dataField(currentRow(),currentColumn());
}

IDataField DataTableWidget::dataField(int ARow, int AColumn) const
{
	IDataField field;
	QTableWidgetItem *item = QTableWidget::item(ARow,AColumn);
	if (item)
	{
		int col = item->data(DTR_COL_INDEX).toInt();
		int row = item->data(DTR_ROW_INDEX).toInt();
		field = FTable.columns.value(col);
		field.value = FTable.rows.value(row).value(col);
	}
	return field;
}

IDataField DataTableWidget::dataField(int ARow, const QString &AVar) const
{
	return dataField(ARow,FDataForms->fieldIndex(AVar,FTable.columns));
}

