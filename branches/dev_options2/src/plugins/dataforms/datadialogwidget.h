#ifndef DATADIALOGWIDGET_H
#define DATADIALOGWIDGET_H

#include <interfaces/idataforms.h>

class DataDialogWidget :
	public QDialog,
	public IDataDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IDataDialogWidget);
public:
	DataDialogWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent);
	~DataDialogWidget();
	virtual QDialog *instance() { return this; }
	virtual ToolBarChanger *toolBarChanged() const;
	virtual QDialogButtonBox *dialogButtons() const;
	virtual IDataFormWidget *formWidget() const;
	virtual void setForm(const IDataForm &AForm);
	virtual bool allowInvalid() const;
	virtual void setAllowInvalid(bool AAllowInvalid);
signals:
	void formWidgetCreated(IDataFormWidget *AForm);
	void formWidgetDestroyed(IDataFormWidget *AForm);
	void dialogDestroyed(IDataDialogWidget *ADialog);
public:
   virtual QSize sizeHint() const;
protected slots:
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	IDataForms *FDataForms;
private:
	bool FAllowInvalid;
	QWidget *FFormHolder;
	IDataFormWidget *FFormWidget;
	ToolBarChanger *FToolBarChanger;
	QDialogButtonBox *FDialogButtons;
};

#endif // DATADIALOGWIDGET_H
