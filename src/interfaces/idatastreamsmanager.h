#ifndef IDATASTREAMSMANAGER_H
#define IDATASTREAMSMANAGER_H

#include <QMap>
#include <QVariant>
#include <QIODevice>

class Jid;
class Stanza;

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
signals:
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
  virtual QWidget *settingsWidget(IDataStreamSocket *ASocket, bool AReadOnly) =0;
  virtual QWidget *settingsWidget(const QString &ASettingsNS, bool AReadOnly)=0;
  virtual void loadSettings(IDataStreamSocket *ASocket, QWidget *AWidget) const=0;
  virtual void loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS) const=0;
  virtual void saveSettings(const QString &ASettingsNS, QWidget *AWidget) =0;
  virtual void saveSettings(const QString &ASettingsNS, IDataStreamSocket *ASocket) =0;
  virtual void deleteSettings(const QString &ASettingsNS) =0;
signals:
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
  virtual QList<QString> methodSettings() const =0;
  virtual QString methodSettingsName(const QString &ASettingsNS) const =0;
  virtual void insertMethodSettings(const QString &ASettingsNS, const QString &ASettingsName) =0;
  virtual void removeMethodSettings(const QString &ASettingsNS) =0;
  virtual QList<QString> profiles() const =0;
  virtual IDataStreamProfile *profile(const QString &AProfileNS) =0;
  virtual void insertProfile(IDataStreamProfile *AProfile) =0;
  virtual void removeProfile(IDataStreamProfile *AProfile) =0;
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, 
    const QList<QString> &AMethods, int ATimeout =0) =0;
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS) =0;
  virtual bool rejectStream(const QString &AStreamId, const QString &AError) =0;
signals:
  virtual void methodInserted(IDataStreamMethod *AMethod) =0;
  virtual void methodRemoved(IDataStreamMethod *AMethod) =0;
  virtual void methodSettingsInserted(const QString &ASettingsNS, const QString &ASettingsName) =0;
  virtual void methodSettingsRemoved(const QString &ASettingsNS) =0;
  virtual void profileInserted(IDataStreamProfile *AProfile) =0;
  virtual void profileRemoved(IDataStreamProfile *AProfile) =0;
};

Q_DECLARE_INTERFACE(IDataStreamSocket,"Vacuum.Plugin.IDataStreamSocket/1.0")
Q_DECLARE_INTERFACE(IDataStreamMethod,"Vacuum.Plugin.IDataStreamMethod/1.0")
Q_DECLARE_INTERFACE(IDataStreamProfile,"Vacuum.Plugin.IDataStreamProfile/1.0")
Q_DECLARE_INTERFACE(IDataStreamsManager,"Vacuum.Plugin.IDataStreamsManager/1.0")

#endif  //IDATASTREAMSMANAGER_H
