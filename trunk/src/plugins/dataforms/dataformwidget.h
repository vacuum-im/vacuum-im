#ifndef DATAFORMWIDGET_H
#define DATAFORMWIDGET_H

#include <interfaces/idataforms.h>

class DataFormWidget :
	public QWidget,
	public IDataFormWidget
{
	Q_OBJECT;
	Q_INTERFACES(IDataFormWidget);
public:
	DataFormWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent);
	virtual QWidget *instance() { return this; }
	virtual bool checkForm(bool AAllowInvalid) const;
	virtual IDataTableWidget *tableWidget() const;
	virtual IDataFieldWidget *fieldWidget(int AIndex) const;
	virtual IDataFieldWidget *fieldWidget(const QString &AVar) const;
	virtual IDataForm userDataForm() const;
	virtual const IDataForm &dataForm() const;
signals:
	void cellActivated(int ARow, int AColumn);
	void cellChanged(int ARow, int AColumn, int APrevRow, int APrevColumn);
	void fieldChanged(IDataFieldWidget *AField);
	void fieldFocusIn(IDataFieldWidget *AField, Qt::FocusReason AReason);
	void fieldFocusOut(IDataFieldWidget *AField, Qt::FocusReason AReason);
	void fieldMediaShown(IDataFieldWidget *AField);
	void fieldMediaError(IDataFieldWidget *AField, const XmppError &AError);
protected:
	bool isStretch(IDataFieldWidget *AWidget) const;
	bool insertLayout(const IDataLayout &ALayout, QWidget *AWidget);
protected slots:
	void onFieldChanged();
	void onFieldFocusIn(Qt::FocusReason AReason);
	void onFieldFocusOut(Qt::FocusReason AReason);
	void onFieldMediaShown();
	void onFieldMediaError(const XmppError &AError);
private:
	IDataForms *FDataForms;
private:
	IDataForm FForm;
	IDataTableWidget *FTableWidget;
	QList<IDataFieldWidget *> FFieldWidgets;
};

#endif // DATAFORMWIDGET_H
