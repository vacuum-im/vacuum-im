#ifndef PROXYSETTINGSWIDGET_H
#define PROXYSETTINGSWIDGET_H

#include <QWidget>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_proxysettingswidget.h"

class ProxySettingsWidget : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  ProxySettingsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent);
  ~ProxySettingsWidget();
  virtual QWidget* instance() { return this; }
public slots:
  void apply(OptionsNode ANode);
  void apply();
  void reset();
signals:
  void modified();
  void childApply();
  void childReset();
protected slots:
  void onEditButtonClicked(bool);
  void onProxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  void onProxyRemoved(const QUuid &AProxyId);
private:
  Ui::ProxySettingsWidgetClass ui;
private:
  OptionsNode FOptions;
  IConnectionManager *FManager;
};

#endif // PROXYSETTINGSWIDGET_H
