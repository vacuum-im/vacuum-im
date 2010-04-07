#ifndef MISCOPTIONSWIDGET_H
#define MISCOPTIONSWIDGET_H

#include <QWidget>
#include <definations/version.h>
#include "ui_miscoptionswidget.h"

class MiscOptionsWidget :
  public QWidget
{
  Q_OBJECT;
public:
  MiscOptionsWidget(QWidget *AParent = NULL);
  ~MiscOptionsWidget();
public slots:
  void apply();
signals:
  void applied();
private:
  Ui::MiscOptionsWidgetClass ui;
};

#endif // MISCOPTIONSWIDGET_H
