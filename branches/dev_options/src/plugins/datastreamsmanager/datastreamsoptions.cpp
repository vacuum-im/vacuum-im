#include "datastreamsoptions.h"

#include <QUuid>
#include <QMessageBox>
#include <QInputDialog>

DataStreamsOptions::DataStreamsOptions(IDataStreamsManager *ADataManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FDataManager = ADataManager;
  
  FWidgetLayout = new QVBoxLayout;
  FWidgetLayout->setMargin(0);
  ui.wdtSettings->setLayout(FWidgetLayout);

  foreach(QString settingsNS, FDataManager->methodSettings())
    ui.cmbProfile->addItem(FDataManager->methodSettingsName(settingsNS), settingsNS);
  ui.cmbProfile->model()->sort(0, Qt::AscendingOrder);
  ui.cmbProfile->insertItem(0,FDataManager->methodSettingsName(QString::null), QVariant(QString::null));
  ui.cmbProfile->setCurrentIndex(0);

  connect(ui.pbtAddProfile, SIGNAL(clicked(bool)),SLOT(onAddProfileButtonClicked(bool)));
  connect(ui.pbtDeleteProfile, SIGNAL(clicked(bool)),SLOT(onDeleteProfileButtonClicked(bool)));
  connect(ui.cmbProfile, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentProfileChanged(int)));
  
  onCurrentProfileChanged(ui.cmbProfile->currentIndex());
}

DataStreamsOptions::~DataStreamsOptions()
{
  foreach(QString settingsNS, FWidgets.keys())
    qDeleteAll(FWidgets.take(settingsNS).values());
}

void DataStreamsOptions::apply()
{
  QList<QString> oldSettings = FDataManager->methodSettings();
  
  for (int index=0; index<ui.cmbProfile->count(); index++)
  {
    QString settingsNS = ui.cmbProfile->itemData(index).toString();
    QString settingsName = ui.cmbProfile->itemText(index);
    FDataManager->insertMethodSettings(settingsNS, settingsName);
    oldSettings.removeAt(oldSettings.indexOf(settingsNS));

    QMap<QString, QWidget *> &widgets = FWidgets[settingsNS];
    foreach(QString smethodNS, widgets.keys())
    {
      IDataStreamMethod *smethod = FDataManager->method(smethodNS);
      if (smethod)
        smethod->saveSettings(settingsNS, widgets.value(smethodNS));
    }
  }

  foreach(QString settingsNS, oldSettings)
  {
    FDataManager->removeMethodSettings(settingsNS);
  }

  emit optionsAccepted();
}

void DataStreamsOptions::onAddProfileButtonClicked(bool)
{
  QString settingsName = QInputDialog::getText(this,tr("Add Profile"),tr("Enter profile name:"));
  if (!settingsName.isEmpty())
  {
    QString settingsNS = QUuid::createUuid().toString();
    ui.cmbProfile->addItem(settingsName, settingsNS);
    ui.cmbProfile->setCurrentIndex(ui.cmbProfile->findData(settingsNS));
  }
}

void DataStreamsOptions::onDeleteProfileButtonClicked(bool)
{
  if (!FSettingsNS.isEmpty())
  {
    QMessageBox::StandardButton button = QMessageBox::warning(this,tr("Delete Profile"),tr("Do you really want to delete a current data streams profile?"),QMessageBox::Yes|QMessageBox::No);
    if (button == QMessageBox::Yes)
    {
      qDeleteAll(FWidgets.take(FSettingsNS).values());
      ui.cmbProfile->removeItem(ui.cmbProfile->currentIndex());
    }
  }
}

void DataStreamsOptions::onCurrentProfileChanged(int AIndex)
{
  foreach(QWidget *widget, FWidgets.value(FSettingsNS))
  {
    FWidgetLayout->removeWidget(widget);
    widget->setParent(NULL);
  }

  FSettingsNS = ui.cmbProfile->itemData(AIndex).toString();

  foreach(QString smethodNS, FDataManager->methods())
  {
    QWidget *widget = FWidgets[FSettingsNS].value(smethodNS);
    if (!widget)
    {
      IDataStreamMethod *smethod = FDataManager->method(smethodNS);
      if (smethod)
      {
        widget = smethod->settingsWidget(FSettingsNS, false);
        FWidgets[FSettingsNS].insert(smethodNS, widget);
      }
    }
    if (widget)
    {
      FWidgetLayout->addWidget(widget);
    }
  }

  if (AIndex != 0)
  {
    ui.cmbProfile->setEditable(true);
    connect(ui.cmbProfile->lineEdit(),SIGNAL(editingFinished()),SLOT(onProfileEditingFinished()));
  }
  else if (ui.cmbProfile->lineEdit() != NULL)
  {
    disconnect(ui.cmbProfile->lineEdit(),SIGNAL(editingFinished()),this,SLOT(onProfileEditingFinished()));
    ui.cmbProfile->setEditable(false);
  }
  ui.pbtDeleteProfile->setEnabled(AIndex != 0);
}

void DataStreamsOptions::onProfileEditingFinished()
{
  QString settingsName = ui.cmbProfile->currentText();
  if (!settingsName.isEmpty())
    ui.cmbProfile->setItemText(ui.cmbProfile->currentIndex(),settingsName);
}