#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <QCheckBox>
#include "../../definations/namespaces.h"
#include "../../definations/discofeatureorder.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iregistraton.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/stanza.h"
#include "registerdialog.h"
#include "registerstream.h"

class Registration : 
  public QObject,
  public IPlugin,
  public IRegistration,
  public IIqStanzaOwner,
  public IDiscoFeatureHandler,
  public IStreamFeaturePlugin,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRegistration IIqStanzaOwner IDiscoFeatureHandler IStreamFeaturePlugin IOptionsHolder);
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
  //IStreamFeaturePlugin
  virtual QList<QString> streamFeatures() const { return QList<QString>() << NS_FEATURE_REGISTER; }
  virtual IStreamFeature *getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
  virtual void destroyStreamFeature(IStreamFeature *AFeature);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IRegistration
  virtual QString sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword);
  virtual QString sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit);
  virtual void showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL);
signals:
  //IStreamFeaturePlugin
  virtual void featureCreated(IStreamFeature *AStreamFeature);
  virtual void featureDestroyed(IStreamFeature *AStreamFeature);
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
  //IRegistration
  virtual void registerFields(const QString &AId, const IRegisterFields &AFields);
  virtual void registerSuccessful(const QString &AId);
  virtual void registerError(const QString &AId, const QString &AError);
protected:
  void registerDiscoFeatures();
protected slots:
  void onRegisterActionTriggered(bool);
  void onStreamDestroyed(IXmppStream *AXmppStream);
  void onOptionsAccepted();
  void onOptionsRejected();
  void onOptionsDialogClosed();
private:
  IDataForms *FDataForms;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  IPresencePlugin *FPresencePlugin;
  ISettingsPlugin *FSettingsPlugin;
  IAccountManager *FAccountManager;
private:
  QList<QString> FSendRequests;
  QList<QString> FSubmitRequests;
  QHash<QString,QCheckBox *> FOptionWidgets;
  QHash<IXmppStream *, IStreamFeature *> FStreamFeatures;
};

#endif // REGISTRATION_H
