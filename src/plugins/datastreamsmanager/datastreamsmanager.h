#ifndef DATASTREAMSMANAGER_H
#define DATASTREAMSMANAGER_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../utils/jid.h"
#include "../../utils/stanza.h"
#include "../../utils/errorhandler.h"

struct StreamParams 
{
  Jid streamJid;
  Jid contactJid;
  QString requestId;
  QString profile;
  IDataForm features;
};

class DataStreamsManger : 
  public QObject,
  public IPlugin,
  public IDataStreamsManager,
  public IStanzaHandler,
  public IIqStanzaOwner
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IDataStreamsManager IStanzaHandler IIqStanzaOwner);
public:
  DataStreamsManger();
  ~DataStreamsManger();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return DATASTREAMSMANAGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &AAccept);
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IDataStreamInitiator
  virtual QList<QString> methods() const;
  virtual IDataStreamMethod *method(const QString &AMethodNS) const;
  virtual void insertMethod(IDataStreamMethod *AMethod, const QString &AMethodNS);
  virtual void removeMethod(IDataStreamMethod *AMethod, const QString &AMethodNS);
  virtual QList<QString> profiles() const;
  virtual IDataStreamProfile *profile(const QString &AProfileNS);
  virtual void insertProfile(IDataStreamProfile *AProfile, const QString &AProfileNS);
  virtual void removeProfile(IDataStreamProfile *AProfile, const QString &AProfileNS);
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, 
    const QList<QString> &AMethods, int ATimeout =0);
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS);
  virtual bool rejectStream(const QString &AStreamId, const QString &AError);
signals:
  virtual void methodInserted(IDataStreamMethod *AMethod, const QString &AMethodNS);
  virtual void methodRemoved(IDataStreamMethod *AMethod, const QString &AMethodNS);
  virtual void profileInserted(IDataStreamProfile *AProfile, const QString &AProfileNS);
  virtual void profileRemoved(IDataStreamProfile *AProfile, const QString &AProfileNS);
protected:
  virtual Stanza errorStanza(const Jid &AContactJid, const QString &ARequestId, const QString &ACondition, 
    const QString &AErrNS=EHN_DEFAULT, const QString &AText=QString::null) const;
  virtual QString streamIdByRequestId(const QString &ARequestId) const;
protected slots:
  void onXmppStreamClosed(IXmppStream *AXmppStream);
private:
  IDataForms *FDataForms;
  IXmppStreams *FXmppStreams;
  IServiceDiscovery *FDiscovery;
  IStanzaProcessor *FStanzaProcessor;
private:
  int FSHIInitStream;
private:
  QMap<QString, StreamParams> FStreams;
  QMap<QString, IDataStreamMethod *> FMethods;
  QMap<QString, IDataStreamProfile *> FProfiles;
};

#endif // DATASTREAMSMANAGER_H
