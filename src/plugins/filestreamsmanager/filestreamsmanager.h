#ifndef FILETSTREAMSMANAGER_H
#define FILETSTREAMSMANAGER_H

#include "../../definations/namespaces.h"
#include "../../definations/menuicons.h"
#include "../../definations/resources.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../utils/jid.h"
#include "../../utils/stanza.h"
#include "../../utils/datetime.h"
#include "../../utils/iconstorage.h"
#include "filestream.h"

class FileStreamsManager : 
  public QObject,
  public IPlugin,
  public IFileStreamsManager,
  public IDataStreamProfile
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IFileStreamsManager IDataStreamProfile);
public:
  FileStreamsManager();
  ~FileStreamsManager();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return FILESTREAMSMANAGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IDataStreamProfile
  virtual bool requestDataStream(const QString &AStreamId, Stanza &ARequest) const;
  virtual bool responceDataStream(const QString &AStreamId, Stanza &AResponce) const;
  virtual bool dataStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
  virtual bool dataStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethod);
  virtual bool dataStreamError(const QString &AStreamId, const QString &AError);
  //IFileTransferManager
  virtual QList<IFileStream *> streams() const;
  virtual IFileStream *streamById(const QString &ASessionId) const;
  virtual IFileStream *createStream(IFileStreamsHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid, 
    const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent = NULL);
  virtual QString defaultStreamMethod() const;
  virtual void setDefaultStreamMethod(const QString &AMethodNS);
  virtual QList<QString> streamMethods() const;
  virtual void insertStreamMethod(const QString &AMethodNS);
  virtual void removeStreamMethod(const QString &AMethodNS);
  virtual IFileStreamsHandler *streamHandler(const QString &AStreamId) const;
  virtual void insertStreamsHandler(IFileStreamsHandler *AHandler, int AOrder);
  virtual void removeStreamsHandler(IFileStreamsHandler *AHandler, int AOrder);
signals:
  virtual void streamCreated(IFileStream *AStream);
  virtual void streamDestroyed(IFileStream *AStream);
protected slots:
  void onStreamDestroyed();
private:
  IDataStreamsManager *FDataManager;
private:
  QString FDefaultMethod;
  QList<QString> FMethods;
  QMap<QString, IFileStream *> FStreams;
  QMultiMap<int, IFileStreamsHandler *> FHandlers;
  QMap<QString, IFileStreamsHandler *> FStreamHandler;
};

#endif // FILETSTREAMSMANAGER_H
