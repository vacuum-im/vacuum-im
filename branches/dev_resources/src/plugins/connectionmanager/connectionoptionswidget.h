#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include "../../interfaces/iconnectionmanager.h"
#include "ui_connectionoptionswidget.h"

class ConnectionOptionsWidget :
  public QWidget
{
  Q_OBJECT;

public:
  ConnectionOptionsWidget(IConnectionManager *AManager, const QString &AAccountId, const QUuid &APluginId);
  ~ConnectionOptionsWidget();

  QString accountId() const { return FAccountId; }
  QUuid pluginId() const { return FCurPluginId; }
protected:
  void setPluginById(const QUuid &APluginId);
protected slots:
  void onComboConnectionsChanged(int AIndex);
private:
  IConnectionManager *FManager;
private:
  Ui::ConnectionOptionsWidgetClass ui;
  QString FAccountId;
  QWidget *FOptionsWidget;
  QUuid FCurPluginId;
};

#endif // CONNECTIONOPTIONSWIDGET_H
