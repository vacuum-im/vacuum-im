#ifndef DATASTREAMSMANAGER_H
#define DATASTREAMSMANAGER_H

#include <definations/namespaces.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/isettings.h>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/errorhandler.h>
#include "datastreamsoptions.h"

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
  public IStanzaRequestOwner,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IDataStreamsManager IStanzaHandler IStanzaRequestOwner IOptionsHolder);
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
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IStanzaHandler
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStreamId);
  //IDataStreamsManager
  virtual QList<QString> methods() const;
  virtual IDataStreamMethod *method(const QString &AMethodNS) const;
  virtual void insertMethod(IDataStreamMethod *AMethod);
  virtual void removeMethod(IDataStreamMethod *AMethod);
  virtual QList<QString> methodSettings() const;
  virtual QString methodSettingsName(const QString &ASettingsNS) const;
  virtual void insertMethodSettings(const QString &ASettingsNS, const QString &ASettingsName);
  virtual void removeMethodSettings(const QString &ASettingsNS);
  virtual QList<QString> profiles() const;
  virtual IDataStreamProfile *profile(const QString &AProfileNS);
  virtual void insertProfile(IDataStreamProfile *AProfile);
  virtual void removeProfile(IDataStreamProfile *AProfile);
  virtual bool initStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId, const QString &AProfileNS, 
    const QList<QString> &AMethods, int ATimeout =0);
  virtual bool acceptStream(const QString &AStreamId, const QString &AMethodNS);
  virtual bool rejectStream(const QString &AStreamId, const QString &AError);
signals:
  virtual void methodInserted(IDataStreamMethod *AMethod);
  virtual void methodRemoved(IDataStreamMethod *AMethod);
  virtual void methodSettingsInserted(const QString &ASettingsNS, const QString &ASettingsName);
  virtual void methodSettingsRemoved(const QString &ASettingsNS);
  virtual void profileInserted(IDataStreamProfile *AProfile);
  virtual void profileRemoved(IDataStreamProfile *AProfile);
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  virtual Stanza errorStanza(const Jid &AContactJid, const QString &ARequestId, const QString &ACondition, 
    const QString &AErrNS=EHN_DEFAULT, const QString &AText=QString::null) const;
  virtual QString streamIdByRequestId(const QString &ARequestId) const;
protected slots:
  void onXmppStreamClosed(IXmppStream *AXmppStream);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IDataForms *FDataForms;
  IXmppStreams *FXmppStreams;
  IServiceDiscovery *FDiscovery;
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
private:
  int FSHIInitStream;
private:
  QMap<QString, StreamParams> FStreams;
  QMap<QString, QString> FMethodSettings;
  QMap<QString, IDataStreamMethod *> FMethods;
  QMap<QString, IDataStreamProfile *> FProfiles;
};

#endif // DATASTREAMSMANAGER_H
