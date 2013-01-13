#ifndef DATASTREAMSOPTIONS_H
#define DATASTREAMSOPTIONS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QObjectCleanupHandler>
#include <definations/optionvalues.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_datastreamsoptions.h"

class DataStreamsOptions : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  DataStreamsOptions(IDataStreamsManager *ADataManager, QWidget *AParent);
  ~DataStreamsOptions();
  virtual QWidget* instance() { return this; }
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
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
  QUuid FCurProfileId;
  QList<QUuid> FNewProfiles;
  QVBoxLayout *FWidgetLayout;
  QObjectCleanupHandler FCleanupHandler;
  QMap<QUuid, QMap<QString, IOptionsWidget *> > FMethodWidgets;
};

#endif // DATASTREAMSOPTIONS_H
