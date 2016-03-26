#ifndef IDATASTREAMSMANAGER_H
#define IDATASTREAMSMANAGER_H

#include <QMap>
#include <QUuid>
#include <QVariant>
#include <QIODevice>
#include <interfaces/idataforms.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/xmpperror.h>

#define DATASTREAMSMANAGER_UUID "{b293dfe1-d8c3-445f-8e7f-b94cc78ec51b}"

struct IDataStream
{
	enum Kind {
		Initiator,
		Target
	};

	int kind;
	Jid streamJid;
	Jid contactJid;
	QString streamId;
	QString requestId;
	QString profileNS;
	IDataForm features;

	IDataStream() {
		kind = Initiator;
	}
	inline bool isNull() const {
		return streamId.isEmpty();
	}
	inline bool isValid() const {
		return !streamId.isEmpty() && streamJid.isValid() && contactJid.isValid() && !profileNS.isEmpty();
	}
};

class IDataStreamSocket
{
public:
	enum State {
		Closed,
		Opening,
		Opened,
		Closing
	};
public:
	virtual QIODevice *instance() =0;
	virtual QString methodNS() const =0;
	virtual QString streamId() const =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual int streamKind() const =0;
	virtual int streamState() const =0;
	virtual XmppError error() const =0;
	virtual bool isOpen() const =0;
	virtual bool open(QIODevice::OpenMode AMode) =0;
	virtual bool flush() =0;
	virtual void close() =0;
	virtual void abort(const XmppError &AError) =0;
protected:
	virtual void stateChanged(int AState) =0;
};

class IDataStreamMethod
{
public:
	virtual QString methodNS() const =0;
	virtual QString methodName() const =0;
	virtual QString methodDescription() const =0;
	virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, IDataStream::Kind AKind, QObject *AParent = NULL) =0;
	virtual IOptionsDialogWidget *methodSettingsWidget(const OptionsNode &ANode, QWidget *AParent) =0;
	virtual void loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode) =0;
protected:
	virtual void socketCreated(IDataStreamSocket *ASocket) =0;
};

class IDataStreamProfile
{
public:
	virtual QString dataStreamProfile() const =0;
	virtual bool dataStreamMakeRequest(const QString &AStreamId, Stanza &ARequest) const =0;
	virtual bool dataStreamMakeResponse(const QString &AStreamId, Stanza &AResponse) const =0;
	virtual bool dataStreamProcessRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods) =0;
	virtual bool dataStreamProcessResponse(const QString &AStreamId, const Stanza &AResponse, const QString &AMethodNS) =0;
	virtual bool dataStreamProcessError(const QString &AStreamId, const XmppError &AError) =0;
};

class IDataStreamsManager
{
public:
	virtual QObject *instance() =0;
	// Methods
	virtual QList<QString> methods() const =0;
	virtual IDataStreamMethod *method(const QString &AMethodNS) const =0;
	virtual void insertMethod(IDataStreamMethod *AMethod) =0;
	virtual void removeMethod(IDataStreamMethod *AMethod) =0;
	// Profiles
	virtual QList<QString> profiles() const =0;
	virtual IDataStreamProfile *profile(const QString &AProfileNS) =0;
	virtual void insertProfile(IDataStreamProfile *AProfile) =0;
	virtual void removeProfile(IDataStreamProfile *AProfile) =0;
	// Settings
	virtual QList<QUuid> settingsProfiles() const =0;
	virtual QString settingsProfileName(const QUuid &ASettingsId) const =0;
	virtual OptionsNode settingsProfileNode(const QUuid &ASettingsId, const QString &AMethodNS) const =0;
	virtual void insertSettingsProfile(const QUuid &ASettingsId, const QString &AName) =0;
	virtual void removeSettingsProfile(const QUuid &ASettingsId) =0;
	// Streams
	virtual bool initStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, const QString &AProfileNS, const QList<QString> &AMethods, int ATimeout=0) =0;
	virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS) =0;
	virtual void rejectStream(const QString &AStreamId, const XmppStanzaError &AError) =0;
protected:
	virtual void methodInserted(IDataStreamMethod *AMethod) =0;
	virtual void methodRemoved(IDataStreamMethod *AMethod) =0;
	virtual void profileInserted(IDataStreamProfile *AProfile) =0;
	virtual void profileRemoved(IDataStreamProfile *AProfile) =0;
	virtual void settingsProfileInserted(const QUuid &ASettingsId) =0;
	virtual void settingsProfileRemoved(const QUuid &ASettingsId) =0;
	virtual void streamInitStarted(const IDataStream &AStream) =0;
	virtual void streamInitFinished(const IDataStream &AStream, const XmppError &AError) =0;
};

Q_DECLARE_INTERFACE(IDataStreamSocket,"Vacuum.Plugin.IDataStreamSocket/1.1")
Q_DECLARE_INTERFACE(IDataStreamMethod,"Vacuum.Plugin.IDataStreamMethod/1.2")
Q_DECLARE_INTERFACE(IDataStreamProfile,"Vacuum.Plugin.IDataStreamProfile/1.2")
Q_DECLARE_INTERFACE(IDataStreamsManager,"Vacuum.Plugin.IDataStreamsManager/1.3")

#endif  //IDATASTREAMSMANAGER_H
