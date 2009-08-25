#ifndef IDATASTREAMSMANAGER_H
#define IDATASTREAMSMANAGER_H

#include <QMap>
#include <QVariant>
#include <QIODevice>

class Jid;
class Stanza;

#define DATASTREAMSMANAGER_UUID "{b293dfe1-d8c3-445f-8e7f-b94cc78ec51b}"

struct IDataStreamOptions
{
  QString methodNS;
  QMap<QString, QVariant> extended;
};

class IDataStreamSocket
{
public:
  enum StreamKind {
    Initiator,
    Target
  };
  enum SocketState {
    Closed,
    Opening,
    Opened,
    Closing
  };
public:
  virtual QIODevice *instance() =0;
  virtual QString socketId() const =0;
  virtual Jid streamJid() const =0;
  virtual Jid contactJid() const =0;
  virtual int socketState() =0;
  virtual bool isOpen() const =0;
  virtual void open(StreamKind AKind, QIODevice::OpenMode AMode) =0;
  virtual bool flush() =0;
  virtual void close() =0;
signals:
  virtual void stateChanged(int AState) =0;
};

class IDataStreamMethod
{
public:
  virtual QString methodName(const QString &AMethodNS) const =0;
  virtual QString methodDescription(const QString &AMethodNS) const =0;
  virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, 
    const Jid &AContactJid, const IDataStreamOptions &AOptions, QObject *AParent =NULL) =0;
  virtual IDataStreamOptions dataStreamOptions(const QString &AMethodNS, const QString &AOptionsNS = QString::null) =0;
};

class IDataStreamProfile
{
public:
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
  virtual void insertMethod(IDataStreamMethod *AMethod, const QString &AMethodNS) =0;
  virtual void removeMethod(IDataStreamMethod *AMethod, const QString &AMethodNS) =0;
  virtual QList<QString> profiles() const =0;
  virtual IDataStreamProfile *profile(const QString &AProfileNS) =0;
  virtual void insertProfile(IDataStreamProfile *AProfile, const QString &AProfileNS) =0;
  virtual void removeProfile(IDataStreamProfile *AProfile, const QString &AProfileNS) =0;
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, 
    const QList<QString> &AMethods, int ATimeout =0) =0;
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS) =0;
  virtual bool rejectStream(const QString &AStreamId, const QString &AError) =0;
signals:
  virtual void methodInserted(IDataStreamMethod *AMethod, const QString &AMethodNS) =0;
  virtual void methodRemoved(IDataStreamMethod *AMethod, const QString &AMethodNS) =0;
  virtual void profileInserted(IDataStreamProfile *AProfile, const QString &AProfileNS) =0;
  virtual void profileRemoved(IDataStreamProfile *AProfile, const QString &AProfileNS) =0;
};

Q_DECLARE_INTERFACE(IDataStreamMethod,"Vacuum.Plugin.IDataStreamMethod/1.0")
Q_DECLARE_INTERFACE(IDataStreamProfile,"Vacuum.Plugin.IDataStreamProfile/1.0")
Q_DECLARE_INTERFACE(IDataStreamsManager,"Vacuum.Plugin.IDataStreamsManager/1.0")

#endif  //IDATASTREAMSMANAGER_H
