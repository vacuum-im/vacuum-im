#ifndef MESSAGESTYLES_H
#define MESSAGESTYLES_H

#include "../../definations/optionnodes.h"
#include "../../definations/optionnodeorders.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/vcardvaluenames.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagestyles.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iavatars.h"
#include "../../interfaces/ivcard.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../utils/message.h"
#include "styleoptionswidget.h"

class MessageStyles : 
  public QObject,
  public IPlugin,
  public IMessageStyles,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageStyles IOptionsHolder);
public:
  MessageStyles();
  ~MessageStyles();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MESSAGESTYLES_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IMessageStyles
  virtual QList<QString> stylePlugins() const;
  virtual IMessageStylePlugin *stylePluginById(const QString &APluginId) const;
  virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const;
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const;
  virtual void setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext = QString::null);
  //Other functions
  virtual QString userAvatar(const Jid &AContactJid) const;
  virtual QString userName(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const;
  virtual QString userIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const;
  virtual QString userIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QString timeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const;
signals:
  virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const;
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected slots:
  void onVCardChanged(const Jid &AContactJid);
private:
  ISettingsPlugin *FSettingsPlugin;
  IAvatars *FAvatars;
  IStatusIcons *FStatusIcons;
  IVCardPlugin *FVCardPlugin;
  IRosterPlugin *FRosterPlugin;
private:
  QMap<QString, IMessageStylePlugin *> FStylePlugins;
  mutable QHash<Jid, QString> FStreamNames;
};

#endif // MESSAGESTYLES_H
