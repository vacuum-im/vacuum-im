#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include "../../definations/version.h"
#include "../../interfaces/iclientinfo.h"
#include "ui_aboutbox.h"

class AboutBox : 
  public QDialog
{
  Q_OBJECT;
public:
  AboutBox(IClientInfo *AClientInfo, QWidget *AParent = NULL);
  ~AboutBox();
private:
  Ui::AboutBoxClass ui;
};

#endif // ABOUTBOX_H
