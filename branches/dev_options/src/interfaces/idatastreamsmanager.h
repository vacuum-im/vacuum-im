#ifndef IDATASTREAMSMANAGER_H
#define IDATASTREAMSMANAGER_H

#include <QMap>
#include <QUuid>
#include <QVariant>
#include <QIODevice>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/options.h>

#define DATASTREAMSMANAGER_UUID "{b293dfe1-d8c3-445f-8e7f-b94cc78ec51b}"

class IDataStreamSocket
{
public:
  enum StreamKind {
    Initiator,
    Target
  };
  enum StreamState {
    Closed,
    Opening,
    Opened,
    Closing
  };
  enum StreamError {
    NoError         = -1,
    UnknownError    = 0
  };
public:
  virtual QIODevice *instance() =0;
  virtual QString methodNS() const =0;
  virtual QString streamId() const =0;
  virtual Jid streamJid() const =0;
  virtual Jid contactJid() const =0;
  virtual int streamKind() const =0;
  virtual int streamState() const =0;
  virtual bool isOpen() const =0;
  virtual bool open(QIODevice::OpenMode AMode) =0;
  virtual bool flush() =0;
  virtual void close() =0;
  virtual void abort(const QString &AError, int ACode = UnknownError) =0;
  virtual int errorCode() const =0;
  virtual QString errorString() const =0;
protected:
  virtual void stateChanged(int AState) =0;
};

class IDataStreamMethod
{
public:
  virtual QString methodNS() const =0;
  virtual QString methodName() const =0;
  virtual QString methodDescription() const =0;
  virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, 
    const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent = NULL) =0;
  virtual IOptionsWidget *methodSettingsWidget(const OptionsNode &ANode, bool AReadOnly, QWidget *AParent) =0;
  virtual IOptionsWidget *methodSettingsWidget(IDataStreamSocket *ASocket, bool AReadOnly, QWidget *AParent) =0;
  virtual void saveMethodSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
  virtual void loadMethodSettings(IDataStreamSocket *ASocket, IOptionsWidget *AWidget) =0;
  virtual void loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode) =0;
protected:
  virtual void socketCreated(IDataStreamSocket *ASocket) =0;
};

class IDataStreamProfile
{
public:
  virtual QString profileNS() const =0;
  virtual bool requestDataStream(const QString &AStreamId, Stanza &ARequest) const =0;
  virtual bool responceDataStream(const QString &AStreamId, Stanza &AResponce) const =0;
  virtual bool dataStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods) =0;
  virtual bool dataStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS) =0;
  virtual bool dataStreamError(const QString &AStreamId, const QString &AError) =0;
};

class IDataStreamsManager 
{
public:
  virtual QObject *instance() =0;
  virtual QList<QString> methods() const =0;
  virtual IDataStreamMethod *method(const QString &AMethodNS) const =0;
  virtual void insertMethod(IDataStreamMethod *AMethod) =0;
  virtual void removeMethod(IDataStreamMethod *AMethod) =0;
  virtual QList<QString> profiles() const =0;
  virtual IDataStreamProfile *profile(const QString &AProfileNS) =0;
  virtual void insertProfile(IDataStreamProfile *AProfile) =0;
  virtual void removeProfile(IDataStreamProfile *AProfile) =0;
  virtual QList<QUuid> settingsProfiles() const =0;
  virtual QString settingsProfileName(const QUuid &AProfileId) const =0;
  virtual OptionsNode settingsProfileNode(const QUuid &AProfileId, const QString &AMethodNS) const =0;
  virtual void insertSettingsProfile(const QUuid &AProfileId, const QString &AName) =0;
  virtual void removeSettingsProfile(const QUuid &AProfileId) =0;
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, 
    const QList<QString> &AMethods, int ATimeout =0) =0;
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS) =0;
  virtual bool rejectStream(const QString &AStreamId, const QString &AError) =0;
protected:
  virtual void methodInserted(IDataStreamMethod *AMethod) =0;
  virtual void methodRemoved(IDataStreamMethod *AMethod) =0;
  virtual void profileInserted(IDataStreamProfile *AProfile) =0;
  virtual void profileRemoved(IDataStreamProfile *AProfile) =0;
  virtual void settingsProfileInserted(const QUuid &AProfileId, const QString &AName) =0;
  virtual void settingsProfileRemoved(const QUuid &AProfileId) =0;
};

Q_DECLARE_INTERFACE(IDataStreamSocket,"Vacuum.Plugin.IDataStreamSocket/1.0")
Q_DECLARE_INTERFACE(IDataStreamMethod,"Vacuum.Plugin.IDataStreamMethod/1.0")
Q_DECLARE_INTERFACE(IDataStreamProfile,"Vacuum.Plugin.IDataStreamProfile/1.0")
Q_DECLARE_INTERFACE(IDataStreamsManager,"Vacuum.Plugin.IDataStreamsManager/1.0")

#endif  //IDATASTREAMSMANAGER_H
