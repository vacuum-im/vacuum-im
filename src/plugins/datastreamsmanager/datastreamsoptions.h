#ifndef DATASTREAMSOPTIONS_H
#define DATASTREAMSOPTIONS_H

#include <QWidget>
#include <QVBoxLayout>
#include "../../interfaces/idatastreamsmanager.h"
#include "ui_datastreamsoptions.h"

class DataStreamsOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  DataStreamsOptions(IDataStreamsManager *ADataManager, QWidget *AParent = NULL);
  ~DataStreamsOptions();
public slots:
  void apply();
signals:
  void optionsAccepted();
protected slots:
  void onAddProfileButtonClicked(bool);
  void onDeleteProfileButtonClicked(bool);
  void onCurrentProfileChanged(int AIndex);
  void onProfileEditingFinished();
private:
  Ui::DataStreamsOptionsClass ui;
private:
  IDataStreamsManager *FDataManager;
private:
  QString FSettingsNS;
  QVBoxLayout *FWidgetLayout;
  QMap<QString, QMap<QString, QWidget *> > FWidgets;
};

#endif // DATASTREAMSOPTIONS_H
