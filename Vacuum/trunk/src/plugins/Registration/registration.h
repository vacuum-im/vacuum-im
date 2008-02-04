#ifndef REGISTRATION_H
#define REGISTRATION_H

#include "../../definations/namespaces.h"
#include "../../definations/discofeatureorder.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iregistraton.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/irostersview.h"
#include "../../utils/stanza.h"
#include "registerdialog.h"

class Registration : 
  public QObject,
  public IPlugin,
  public IRegistration,
  public IIqStanzaOwner,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRegistration IIqStanzaOwner IDiscoFeatureHandler);
public:
  Registration();
  ~Registration();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return REGISTRATION_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IRegistration
  virtual QString sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword);
  virtual QString sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit);
  virtual void showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL);
signals:
  virtual void registerFields(const QString &AId, const IRegisterFields &AFields);
  virtual void registerSuccessful(const QString &AId);
  virtual void registerError(const QString &AId, const QString &AError);
protected:
  void registerDiscoFeatures();
protected slots:
  void onRegisterActionTriggered(bool);
private:
  IDataForms *FDataForms;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
private:
  QList<QString> FSendRequests;
  QList<QString> FSubmitRequests;
};

#endif // REGISTRATION_H
