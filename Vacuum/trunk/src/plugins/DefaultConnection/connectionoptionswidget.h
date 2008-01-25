#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include "ui_connectionoptionswidget.h"

class ConnectionOptionsWidget : 
  public QWidget
{
  Q_OBJECT;

public:
  ConnectionOptionsWidget();
  ~ConnectionOptionsWidget();

  QString host() const;
  void setHost(const QString &AHost);
  int port() const;
  void setPort(int APort);
  bool useSSL() const;
  void setUseSSL(bool AUseSSL);
  bool ignoreSSLErrors() const;
  void setIgnoreSSLError(bool AIgnore);
  int proxyType() const;
  void setProxyTypes(const QStringList &AProxyTypes);
  void setProxyType(int AProxyType);
  QString proxyHost() const;
  void setProxyHost(const QString &AProxyHost);
  int proxyPort() const;
  void setProxyPort(int AProxyPort);
  QString proxyUserName() const;
  void setProxyUserName(const QString &AProxyUser);
  QString proxyPassword() const;
  void setProxyPassword(const QString &APassword);
protected slots:
  void onUseSSLStateChanged(int AState);
  void onProxyTypeChanged(int AIndex);
private:
    Ui::ConnectionOptionsWidgetClass ui;
};

#endif // CONNECTIONOPTIONSWIDGET_H
