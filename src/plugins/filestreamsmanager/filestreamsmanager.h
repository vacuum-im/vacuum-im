#ifndef FILETSTREAMSMANAGER_H
#define FILETSTREAMSMANAGER_H

#include <QPointer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/itraymanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include "filestream.h"
#include "filestreamswindow.h"
#include "filestreamsoptionswidget.h"

class FileStreamsManager :
	public QObject,
	public IPlugin,
	public IFileStreamsManager,
	public IDataStreamProfile,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsDialogHolder IFileStreamsManager IDataStreamProfile);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.FileStreamsManager");
public:
	FileStreamsManager();
	~FileStreamsManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return FILESTREAMSMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IDataStreamProfile
	virtual QString dataStreamProfile() const;
	virtual bool dataStreamMakeRequest(const QString &AStreamId, Stanza &ARequest) const;
	virtual bool dataStreamMakeResponse(const QString &AStreamId, Stanza &AResponse) const;
	virtual bool dataStreamProcessRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
	virtual bool dataStreamProcessResponse(const QString &AStreamId, const Stanza &AResponse, const QString &AMethod);
	virtual bool dataStreamProcessError(const QString &AStreamId, const XmppError &AError);
	//IFileTransferManager
	virtual QList<IFileStream *> streams() const;
	virtual IFileStream *findStream(const QString &ASessionId) const;
	virtual IFileStreamHandler *findStreamHandler(const QString &AStreamId) const;
	virtual IFileStream *createStream(IFileStreamHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent = NULL);
	// Stream Handlers
	virtual QList<IFileStreamHandler *> streamHandlers() const;
	virtual void insertStreamsHandler(int AOrder, IFileStreamHandler *AHandler);
	virtual void removeStreamsHandler(int AOrder, IFileStreamHandler *AHandler);
signals:
	void streamCreated(IFileStream *AStream);
	void streamDestroyed(IFileStream *AStream);
	void streamHandlerInserted(int AOrder, IFileStreamHandler *AHandler);
	void streamHandlerRemoved(int AOrder, IFileStreamHandler *AHandler);
protected slots:
	void onStreamDestroyed();
	void onShowFileStreamsWindow(bool);
	void onProfileClosed(const QString &AName);
private:
	IDataStreamsManager *FDataManager;
	IOptionsManager *FOptionsManager;
	ITrayManager *FTrayManager;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	QMap<QString, IFileStream *> FStreams;
	QMultiMap<int, IFileStreamHandler *> FHandlers;
	QMap<QString, IFileStreamHandler *> FStreamHandler;
private:
	QPointer<FileStreamsWindow> FFileStreamsWindow;
};

#endif // FILETSTREAMSMANAGER_H
