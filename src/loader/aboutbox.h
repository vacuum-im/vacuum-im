#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include <definations/version.h>
#include <interfaces/ipluginmanager.h>
#include "ui_aboutbox.h"

class AboutBox : 
  public QDialog
{
  Q_OBJECT;
public:
  AboutBox(IPluginManager *APluginManager, QWidget *AParent = NULL);
  ~AboutBox();
private:
  Ui::AboutBoxClass ui;
};

#endif // ABOUTBOX_H
