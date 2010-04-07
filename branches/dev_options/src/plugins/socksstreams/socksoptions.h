#ifndef SOCKSOPTIONS_H
#define SOCKSOPTIONS_H

#include <QWidget>
#include <interfaces/isocksstreams.h>
#include <interfaces/iconnectionmanager.h>
#include "ui_socksoptions.h"

#define PROXY_NS_PREFIX  "SocksStreams"

class SocksOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  SocksOptions(ISocksStreams *ASocksStreams, ISocksStream *ASocksStream, bool AReadOnly, QWidget *AParent = NULL);
  SocksOptions(ISocksStreams *ASocksStreams, IConnectionManager *AConnectionManager, const QString &ASettingsNS, 
    bool AReadOnly, QWidget *AParent = NULL);
  ~SocksOptions();
  void saveSettings(ISocksStream *AStream);
  void saveSettings(const QString &ASettingsNS);
public slots:
  void apply();
signals:
  void optionsAccepted();
protected:
  void initialize(bool AReadOnly);
protected slots:
  void onAddStreamProxyClicked(bool);
  void onStreamProxyUpClicked(bool);
  void onStreamProxyDownClicked(bool);
  void onDeleteStreamProxyClicked(bool);
private:
  Ui::SocksOptionsClass ui;
private:
  ISocksStreams *FSocksStreams;
  IConnectionManager *FConnectionManager;
private:
  QWidget *FProxySettings;
  QString FSettingsNS;
  ISocksStream *FSocksStream;
};

#endif // SOCKSOPTIONS_H
