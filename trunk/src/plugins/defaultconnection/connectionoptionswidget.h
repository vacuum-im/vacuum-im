#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/isettings.h>
#include "ui_connectionoptionswidget.h"

#define SVN_CONNECTION                      "connection[]"
#define SVN_CONNECTION_HOST                 SVN_CONNECTION ":host"
#define SVN_CONNECTION_PORT                 SVN_CONNECTION ":port"
#define SVN_CONNECTION_USE_SSL              SVN_CONNECTION ":useSSL"
#define SVN_CONNECTION_IGNORE_SSLERROR      SVN_CONNECTION ":ingnoreSSLErrors"

class ConnectionOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  ConnectionOptionsWidget(IConnectionManager *AManager, ISettings *ASettings, const QString &ASettingsNS, QWidget *AParent = NULL);
  ~ConnectionOptionsWidget();
public slots:
  void apply(const QString &ASettingsNS);
private:
  Ui::ConnectionOptionsWidgetClass ui;
private:
  ISettings *FSettings;
  IConnectionManager *FManager;
private:
  QString FSettingsNS;
  QWidget *FProxySettings;
};

#endif // CONNECTIONOPTIONSWIDGET_H
