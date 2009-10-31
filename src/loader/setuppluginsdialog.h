#ifndef SETUPPLUGINSDIALOG_H
#define SETUPPLUGINSDIALOG_H

#include <QDialog>
#include <QDomDocument>
#include <definations/menuicons.h>
#include <definations/resources.h>
#include <interfaces/ipluginmanager.h>
#include <utils/iconstorage.h>
#include "ui_setuppluginsdialog.h"

class SetupPluginsDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  SetupPluginsDialog(IPluginManager *APluginManager, QDomDocument APluginsSetup, QWidget *AParent = NULL);
  ~SetupPluginsDialog();
protected:
  void updateLanguage();
  void updatePlugins();
  void saveSettings();
protected slots:
  void onCurrentLanguageChanged(int AIndex);
  void onCurrentPluginChanged(QTableWidgetItem *ACurrent, QTableWidgetItem *APrevious);
  void onDialogButtonClicked(QAbstractButton *AButton);
private:
  Ui::SetupPluginsDialogClass ui;
private:
  IPluginManager *FPluginManager;
private:
  QDomDocument FPluginsSetup;
  QMap<QTableWidgetItem *, QDomElement> FItemElement;
};

#endif // SETUPPLUGINSDIALOG_H
