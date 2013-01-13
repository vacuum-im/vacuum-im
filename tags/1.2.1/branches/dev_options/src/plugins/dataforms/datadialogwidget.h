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
  virtual ToolBarChanger *toolBarChanged() const { return FToolBarChanger; }
  virtual QDialogButtonBox *dialogButtons() const { return FDialogButtons; }
  virtual IDataFormWidget *formWidget() const { return FFormWidget; }
  virtual void setForm(const IDataForm &AForm);
  virtual bool allowInvalid() const { return FAllowInvalid; }
  virtual void setAllowInvalid(bool AAllowInvalid) { FAllowInvalid = AAllowInvalid; }
signals:
  void formWidgetCreated(IDataFormWidget *AForm);
  void formWidgetDestroyed(IDataFormWidget *AForm);
  void dialogDestroyed(IDataDialogWidget *ADialog);
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
