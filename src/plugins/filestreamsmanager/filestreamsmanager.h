#ifndef FILETSTREAMSMANAGER_H
#define FILETSTREAMSMANAGER_H

#include <QPointer>
#include <definitions/namespaces.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/shortcuts.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/internalerrors.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/itraymanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/shortcuts.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
#include "filestream.h"
#include "filestreamswindow.h"
#include "filestreamsoptions.h"

class FileStreamsManager :
	public QObject,
	public IPlugin,
	public IFileStreamsManager,
	public IDataStreamProfile,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder IFileStreamsManager IDataStreamProfile);
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
	// IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IDataStreamProfile
	virtual QString profileNS() const;
	virtual bool requestDataStream(const QString &AStreamId, Stanza &ARequest) const;
	virtual bool responceDataStream(const QString &AStreamId, Stanza &AResponce) const;
	virtual bool dataStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
	virtual bool dataStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethod);
	virtual bool dataStreamError(const QString &AStreamId, const XmppError &AError);
	//IFileTransferManager
	virtual QList<IFileStream *> streams() const;
	virtual IFileStream *streamById(const QString &ASessionId) const;
	virtual IFileStream *createStream(IFileStreamsHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent = NULL);
	virtual IFileStreamsHandler *streamHandler(const QString &AStreamId) const;
	virtual void insertStreamsHandler(IFileStreamsHandler *AHandler, int AOrder);
	virtual void removeStreamsHandler(IFileStreamsHandler *AHandler, int AOrder);
signals:
	void streamCreated(IFileStream *AStream);
	void streamDestroyed(IFileStream *AStream);
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
	QMultiMap<int, IFileStreamsHandler *> FHandlers;
	QMap<QString, IFileStreamsHandler *> FStreamHandler;
private:
	QPointer<FileStreamsWindow> FFileStreamsWindow;
};

#endif // FILETSTREAMSMANAGER_H
