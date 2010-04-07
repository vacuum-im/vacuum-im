#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_connectionoptionswidget.h"

class ConnectionOptionsWidget : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent = NULL);
  ~ConnectionOptionsWidget();
  virtual QWidget* instance() { return this; }
public slots:
  void apply(OptionsNode ANode);
  void apply();
  void reset();
signals:
  void modified();
  void childApply();
  void childReset();
private:
  Ui::ConnectionOptionsWidgetClass ui;
private:
  IConnectionManager *FManager;
private:
  OptionsNode FOptions;
  IOptionsWidget *FProxySettings;
};

#endif // CONNECTIONOPTIONSWIDGET_H
