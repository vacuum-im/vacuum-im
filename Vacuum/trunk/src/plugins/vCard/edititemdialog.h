#ifndef EDITITEMDIALOG_H
#define EDITITEMDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include "ui_edititemdialog.h"

class EditItemDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  EditItemDialog(const QString &AValue, QStringList ATags, QStringList ATagList, QWidget *AParent = NULL);
  ~EditItemDialog();
  QString value() const;
  QStringList tags() const;
  void setLabelText(const QString &AText);
  void setValueEditable(bool AEditable);
  void setTagsEditable(bool AEditable);
private:
  Ui::EditItemDialogClass ui;
private:
  QList<QCheckBox *> FCheckBoxes;
};

#endif // EDITITEMDIALOG_H
