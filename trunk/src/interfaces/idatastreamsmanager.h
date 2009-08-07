#ifndef IDATASTREAMSMANAGER_H
#define IDATASTREAMSMANAGER_H

class QIODevice;
class Jid;
class Stanza;

#define DATASTREAMSMANAGER_UUID "{b293dfe1-d8c3-445f-8e7f-b94cc78ec51b}"

class IDataStream
{
public:
  virtual QIODevice *instance() =0;
};

class IDataStreamMethod
{
public:
  virtual QObject *instance() =0;
};

class IDataStreamProfile
{
public:
  virtual QObject *instance() =0;
  virtual bool insertRequestData(const QString &AStreamId, Stanza &ARequest) const =0;
  virtual bool insertResponceData(const QString &AStreamId, Stanza &AResponce) const =0;
  virtual bool processStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods) =0;
  virtual bool precessStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethod) =0;
  virtual bool processStreamError(const QString &AStreamId, const QString &AError) =0;
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
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfile, 
    const QList<QString> &AMethods, int ATimeout =0) =0;
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethod) =0;
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
