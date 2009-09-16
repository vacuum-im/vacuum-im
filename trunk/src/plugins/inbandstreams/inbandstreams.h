#ifndef INBANDSTREAMS_H
#define INBANDSTREAMS_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iinbandstreams.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../interfaces/istanzaprocessor.h"
#include "inbandstream.h"

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
  virtual void loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS);
  virtual void saveSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS);
private:
  IFileStreamsManager *FFileManager;
  IDataStreamsManager *FDataManager;
  IStanzaProcessor *FStanzaProcessor;
};

#endif // INBANDSTREAMS_H
