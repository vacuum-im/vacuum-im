#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <definations/namespaces.h>
#include <definations/fshandlerorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/notificationdataroles.h>
#include <definations/menuicons.h>
#include <definations/soundfiles.h>
#include <definations/resources.h>
#include <definations/toolbargroups.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifiletransfer.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iroster.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersview.h>
#include <interfaces/isettings.h>
#include <utils/jid.h>
#include <utils/action.h>
#include <utils/stanza.h>
#include <utils/datetime.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
#include "streamdialog.h"
#include "filetransferoptions.h"

class FileTransfer : 
  public QObject,
  public IPlugin,
  public IFileTransfer,
  public IOptionsHolder,
  public IDiscoFeatureHandler,
  public IRostersDragDropHandler,
  public IFileStreamsHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IFileTransfer IOptionsHolder IDiscoFeatureHandler  IRostersDragDropHandler IFileStreamsHandler);
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
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IRostersDragDropHandler
  virtual Qt::DropActions dragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
  virtual bool dragEnter(const QDragEnterEvent *AEvent);
  virtual bool dragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
  virtual void dragLeave(const QDragLeaveEvent *AEvent);
  virtual bool dropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
  //IFileTransferHandler
  virtual bool fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
  virtual bool fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS);
  virtual bool fileStreamShowDialog(const QString &AStreamId);
  //IFileTransfer
  virtual bool autoReceive() const;
  virtual void setAutoReceive(bool AAuto);
  virtual bool hideDialogWhenStarted() const;
  virtual void setHideDialogWhenStarted(bool AHide);
  virtual bool removeTransferWhenFinished() const;
  virtual void setRemoveTransferWhenFinished(bool ARemove);
  virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null, const QString &AFileDesc = QString::null);
signals:
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void registerDiscoFeatures();
  void notifyStream(IFileStream *AStream, bool ANewStream = false);
  void autoStartStream(IFileStream *AStream);
  void insertToolBarAction(IToolBarWidget *AWidget);
  void removeToolBarAction(IToolBarWidget *AWidget);
  QList<IToolBarWidget *> findToolBarWidgets(const Jid &AContactJid) const;
  StreamDialog *createStreamDialog(IFileStream *ASession);
  IFileStream *createStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind);
protected slots:
  void onStreamStateChanged();
  void onStreamDestroyed();
  void onStreamDialogDestroyed();
  void onShowSendFileDialogByAction(bool);
  void onNotificationActivated(int ANotifyId);
  void onNotificationRemoved(int ANotifyId);
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
  void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
  void onToolBarWidgetCreated(IToolBarWidget *AWidget);
  void onEditWidgetContactJidChanged(const Jid &ABefour);
  void onToolBarWidgetDestroyed(QObject *AObject);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IRosterPlugin *FRosterPlugin;
  IServiceDiscovery *FDiscovery;
  INotifications *FNotifications;
  IDataStreamsManager *FDataManager;
  IFileStreamsManager *FFileManager;
  IMessageWidgets *FMessageWidgets;
  ISettingsPlugin *FSettingsPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  bool FAutoReceive;
  bool FHideDialogWhenStarted;
  bool FRemoveTransferWhenFinished;
  QMap<QString, int> FStreamNotify;
  QMap<QString, StreamDialog *> FStreamDialog;
  QMap<IToolBarWidget *, Action *> FToolBarActions;
};

#endif // FILETRANSFER_H
