#ifndef PROXYSETTINGSWIDGET_H
#define PROXYSETTINGSWIDGET_H

#include <QWidget>
#include <interfaces/iconnectionmanager.h>
#include "ui_proxysettingswidget.h"

#define DEFAULT_PROXY_REF_UUID  "{b919d5c9-6def-43ba-87aa-892d49b9ac67}"

class ProxySettingsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  ProxySettingsWidget(IConnectionManager *AManager, const QString &ASettingsNS, QWidget *AParent);
  ~ProxySettingsWidget();
public slots:
  void apply(const QString &ASettingsNS);
protected slots:
  void onEditButtonClicked(bool);
  void onProxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy);
  void onProxyRemoved(const QUuid &AProxyId);
private:
  Ui::ProxySettingsWidgetClass ui;
private:
  QString FSettingsNS;
  IConnectionManager *FManager;
};

#endif // PROXYSETTINGSWIDGET_H
