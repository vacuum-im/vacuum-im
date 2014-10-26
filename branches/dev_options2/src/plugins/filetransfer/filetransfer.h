#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifiletransfer.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iroster.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include "streamdialog.h"

class FileTransfer :
	public QObject,
	public IPlugin,
	public IFileTransfer,
	public IOptionsHolder,
	public IDiscoFeatureHandler,
	public IRostersDragDropHandler,
	public IMessageViewDropHandler,
	public IFileStreamsHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IFileTransfer IOptionsHolder IDiscoFeatureHandler  IRostersDragDropHandler IMessageViewDropHandler IFileStreamsHandler);
public:
	FileTransfer();
	~FileTransfer();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return FILETRANSFER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu);
	//IMessageViewDropHandler
	virtual bool messagaeViewDragEnter(IMessageViewWidget *AWidget, const QDragEnterEvent *AEvent);
	virtual bool messageViewDragMove(IMessageViewWidget *AWidget, const QDragMoveEvent *AEvent);
	virtual void messageViewDragLeave(IMessageViewWidget *AWidget, const QDragLeaveEvent *AEvent);
	virtual bool messageViewDropAction(IMessageViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu);
	//IFileTransferHandler
	virtual bool fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
	virtual bool fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS);
	virtual bool fileStreamShowDialog(const QString &AStreamId);
	//IFileTransfer
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null, const QString &AFileDesc = QString::null);
protected:
	void registerDiscoFeatures();
	void notifyStream(IFileStream *AStream, bool ANewStream = false);
	bool autoStartStream(IFileStream *AStream) const;
	void updateToolBarAction(IMessageToolBarWidget *AWidget);
	QList<IMessageToolBarWidget *> findToolBarWidgets(const Jid &AContactJid) const;
	StreamDialog *getStreamDialog(IFileStream *ASession);
	IFileStream *createStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind);
	QString dirNameByUserName(const QString &AUserName) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onStreamStateChanged();
	void onStreamDestroyed();
	void onStreamDialogDestroyed();
	void onShowSendFileDialogByAction(bool);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
	void onToolBarWidgetCreated(IMessageToolBarWidget *AWidget);
	void onToolBarWidgetAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
	void onToolBarWidgetDestroyed(QObject *AObject);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IRosterPlugin *FRosterPlugin;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
	IDataStreamsManager *FDataManager;
	IFileStreamsManager *FFileManager;
	IMessageWidgets *FMessageWidgets;
	IMessageArchiver *FMessageArchiver;
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QMap<QString, int> FStreamNotify;
	QMap<QString, StreamDialog *> FStreamDialog;
	QMap<IMessageToolBarWidget *, Action *> FToolBarActions;
};

#endif // FILETRANSFER_H
