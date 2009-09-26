#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "../../definations/namespaces.h"
#include "../../definations/fshandlerorders.h"
#include "../../definations/discofeaturehandlerorders.h"
#include "../../definations/notificationdataroles.h"
#include "../../definations/menuicons.h"
#include "../../definations/soundfiles.h"
#include "../../definations/resources.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ifiletransfer.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/inotifications.h"
#include "../../utils/jid.h"
#include "../../utils/action.h"
#include "../../utils/stanza.h"
#include "../../utils/datetime.h"
#include "../../utils/iconstorage.h"
#include "streamdialog.h"

class FileTransfer : 
  public QObject,
  public IPlugin,
  public IFileTransfer,
  public IFileStreamsHandler,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IFileTransfer IFileStreamsHandler IDiscoFeatureHandler);
public:
  FileTransfer();
  ~FileTransfer();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return FILETRANSFER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IFileTransferHandler
  virtual bool fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
  virtual bool fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS);
  virtual bool fileStreamShowDialog(const QString &AStreamId);
  //IFileTransfer
  virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null);
protected:
  void registerDiscoFeatures();
  void notifyStream(IFileStream *AStream);
  StreamDialog *createStreamDialog(IFileStream *ASession);
  IFileStream *createStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind);
protected slots:
  void onStreamStateChanged();
  void onStreamDestroyed();
  void onStreamDialogDestroyed();
  void onShowSendFileDialogByAction(bool);
  void onNotificationActivated(int ANotifyId);
  void onNotificationRemoved(int ANotifyId);
private:
  IServiceDiscovery *FDiscovery;
  INotifications *FNotifications;
  IDataStreamsManager *FDataManager;
  IFileStreamsManager *FFileManager;
private:
  QMap<QString, int> FStreamNotify;
  QMap<QString, StreamDialog *> FStreamDialog;
};

#endif // FILETRANSFER_H
