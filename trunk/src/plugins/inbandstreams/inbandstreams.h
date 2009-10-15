#ifndef INBANDSTREAMS_H
#define INBANDSTREAMS_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iinbandstreams.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/isettings.h"
#include "inbandstream.h"
#include "inbandoptions.h"

class InBandStreams : 
  public QObject,
  public IPlugin,
  public IInBandStreams
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IInBandStreams IDataStreamMethod);
public:
  InBandStreams();
  ~InBandStreams();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return INBANDSTREAMS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IDataStreamMethod
  virtual QString methodNS() const;
  virtual QString methodName() const;
  virtual QString methodDescription() const;
  virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, 
    const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent=NULL);
  virtual QWidget *settingsWidget(IDataStreamSocket *ASocket, bool AReadOnly);
  virtual QWidget *settingsWidget(const QString &ASettingsNS, bool AReadOnly);
  virtual void loadSettings(IDataStreamSocket *ASocket, QWidget *AWidget) const;
  virtual void loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS) const;
  virtual void saveSettings(const QString &ASettingsNS, QWidget *AWidget);
  virtual void saveSettings(const QString &ASettingsNS, IDataStreamSocket *ASocket);
  virtual void deleteSettings(const QString &ASettingsNS);
  //InBandStreams
  virtual int blockSize(const QString &ASettingsNS) const;
  virtual void setBlockSize(const QString &ASettingsNS, int ASize);
  virtual int maximumBlockSize(const QString &ASettingsNS);
  virtual void setMaximumBlockSize(const QString &ASettingsNS, int AMaxSize);
  virtual int dataStanzaType(const QString &ASettingsNS) const;
  virtual void setDataStanzaType(const QString &ASettingsNS, int AType);
signals:
  virtual void socketCreated(IDataStreamSocket *ASocket);
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IFileStreamsManager *FFileManager;
  IDataStreamsManager *FDataManager;
  IStanzaProcessor *FStanzaProcessor;
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;
private:
  int FBlockSize;
  int FMaxBlockSize;
  int FStatnzaType;
};

#endif // INBANDSTREAMS_H
