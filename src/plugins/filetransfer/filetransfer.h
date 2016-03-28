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
#include <interfaces/idatastreamspublisher.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/irostermanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppuriqueries.h>
#include "streamdialog.h"

class FileTransfer :
	public QObject,
	public IPlugin,
	public IFileTransfer,
	public IMessageWriter,
	public IXmppUriHandler,
	public IFileStreamHandler,
	public IOptionsDialogHolder,
	public IDiscoFeatureHandler,
	public IRostersDragDropHandler,
	public IMessageViewDropHandler,
	public IMessageViewUrlHandler,
	public IPublicDataStreamHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IFileTransfer IMessageWriter IXmppUriHandler IFileStreamHandler IOptionsDialogHolder IDiscoFeatureHandler IRostersDragDropHandler IMessageViewDropHandler IMessageViewUrlHandler IPublicDataStreamHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.FileTransfer");
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
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
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
	virtual bool messageViewDragEnter(IMessageViewWidget *AWidget, const QDragEnterEvent *AEvent);
	virtual bool messageViewDragMove(IMessageViewWidget *AWidget, const QDragMoveEvent *AEvent);
	virtual void messageViewDragLeave(IMessageViewWidget *AWidget, const QDragLeaveEvent *AEvent);
	virtual bool messageViewDropAction(IMessageViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu);
	//IMessageViewUrlHandler
	virtual bool messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl);
	//IMessageWriter
	virtual bool writeMessageHasText(int AOrder, Message &AMessage, const QString &ALang);
	virtual bool writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual bool writeTextToMessage(int AOrder, QTextDocument *ADocument, Message &AMessage, const QString &ALang);
	//IPublicDataStreamHandler
	virtual bool publicDataStreamCanStart(const IPublicDataStream &AStream) const;
	virtual bool publicDataStreamStart(const Jid &AStreamJid, const Jid AContactJid, const QString &ASessionId, const IPublicDataStream &AStream);
	virtual bool publicDataStreamRead(IPublicDataStream &AStream, const QDomElement &ASiPub) const;
	virtual bool publicDataStreamWrite(const IPublicDataStream &AStream, QDomElement &ASiPub) const;
	//IFileTransferHandler
	virtual bool fileStreamProcessRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
	virtual bool fileStreamProcessResponse(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS);
	virtual bool fileStreamShowDialog(const QString &AStreamId);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IFileTransfer
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName=QString::null, const QString &AFileDesc=QString::null);
	//Send Public Files
	virtual IPublicFile findPublicFile(const QString &AFileId) const;
	virtual QList<IPublicFile> findPublicFiles(const Jid &AOwnerJid=Jid::null, const QString &AFileName=QString::null) const;
	virtual QString registerPublicFile(const Jid &AOwnerJid, const QString &AFileName, const QString &AFileDesc=QString::null);
	virtual void removePublicFile(const QString &AFileId);
	//Receive Public Files
	virtual QList<IPublicFile> readPublicFiles(const QDomElement &AParent) const;
	virtual bool writePublicFile(const QString &AFileId, QDomElement &AParent) const;
	virtual QString receivePublicFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileId);
signals:
	void publicFileSendStarted(const QString &AFileId, IFileStream *AStream);
	void publicFileReceiveStarted(const QString &ARequestId, IFileStream *AStream);
	void publicFileReceiveRejected(const QString &ARequestId, const XmppError &AError);
protected:
	void notifyStream(IFileStream *AStream);
	bool autoStartStream(IFileStream *AStream) const;
	void updateToolBarAction(IMessageToolBarWidget *AWidget);
	QList<IMessageToolBarWidget *> findToolBarWidgets(const Jid &AContactJid) const;
	StreamDialog *getStreamDialog(IFileStream *ASession);
	IFileStream *createStream(const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind, const QString &AStreamId);
	QString dirNameByUserName(const QString &AUserName) const;
	IPublicFile publicFileFromStream(const IPublicDataStream &AStream) const;
	void showStatusEvent(IMessageViewWidget *AView, const QString &AHtml) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onStreamStateChanged();
	void onStreamDestroyed();
	void onStreamDialogDestroyed();
	void onSendFileByAction(bool);
	void onPublishFilesByAction(bool);
protected slots:
	void onPublicStreamStartAccepted(const QString &ARequestId, const QString &ASessionId);
	void onPublicStreamStartRejected(const QString &ARequestId, const XmppError &AError);
protected slots:
	void onDataStreamInitStarted(const IDataStream &AStream);
	void onDataStreamInitFinished(const IDataStream &AStream, const XmppError &AError);
protected slots:
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
protected slots:
	void onMultiUserChatStateChanged(int AState);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
protected slots:
	void onToolBarWidgetCreated(IMessageToolBarWidget *AWidget);
	void onToolBarWidgetAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
	void onToolBarWidgetDestroyed(QObject *AObject);
protected slots:
	void onMessageViewWidgetDestroyed(QObject *AObject);
private:
	IRosterManager *FRosterManager;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
	IDataStreamsManager *FDataManager;
	IFileStreamsManager *FFileManager;
	IDataStreamsPublisher *FDataPublisher;
	IMessageWidgets *FMessageWidgets;
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageProcessor *FMessageProcessor;
	IXmppUriQueries *FXmppUriQueries;
	IMultiUserChatManager *FMultiChatManager;
private:
	QMap<QString, int> FStreamNotify;
	QMap<QString, StreamDialog *> FStreamDialog;
private:
	QList<IFileStream *> FPublicStreams;
	QList<QString> FPublicReceiveRequests;
	QMap<QString, QString> FPublicReceiveSessions;
	QMap<QString, IMessageViewWidget *> FPublicRequestView;
private:
	QMap<IMessageToolBarWidget *, Action *> FToolBarActions;
};

#endif // FILETRANSFER_H
