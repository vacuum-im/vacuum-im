#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include "../../interfaces/iconnectionmanager.h"
#include "../../interfaces/iaccountmanager.h"
#include "ui_connectionoptionswidget.h"
#include "connectionmanager.h"

class ConnectionManager;

class ConnectionOptionsWidget :
  public QWidget
{
  Q_OBJECT;
public:
  ConnectionOptionsWidget(ConnectionManager *ACManager, IAccountManager *AAManager, const QUuid &AAccountId);
  ~ConnectionOptionsWidget();
public slots:
  void apply();
signals:
  void optionsAccepted();
protected:
  void setPluginById(const QUuid &APluginId);
protected slots:
  void onComboConnectionsChanged(int AIndex);
private:
  IAccountManager *FAccountManager;
  ConnectionManager *FConnectionManager;
private:
  Ui::ConnectionOptionsWidgetClass ui;
private:
  QUuid FPluginId;
  QUuid FAccountId;
  QWidget *FOptionsWidget;
};

#endif // CONNECTIONOPTIONSWIDGET_H
