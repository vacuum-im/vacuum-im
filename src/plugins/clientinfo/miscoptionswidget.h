#ifndef MISCOPTIONSWIDGET_H
#define MISCOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/iclientinfo.h>
#include "ui_miscoptionswidget.h"

class MiscOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  MiscOptionsWidget(IClientInfo *AClientInfo, QWidget *AParent = NULL);
  ~MiscOptionsWidget();
public slots:
  void apply();
signals:
  void optionsAccepted();
private:
  Ui::MiscOptionsWidgetClass ui;
private:
  IClientInfo *FClientInfo;
};

#endif // MISCOPTIONSWIDGET_H
