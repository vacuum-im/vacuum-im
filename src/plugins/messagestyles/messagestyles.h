#ifndef MESSAGESTYLES_H
#define MESSAGESTYLES_H

#include "../../definations/resources.h"
#include "../../definations/vcardvaluenames.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagestyles.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iavatars.h"
#include "../../interfaces/ivcard.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../utils/filestorage.h"
#include "../../utils/message.h"
#include "messagestyle.h"

class MessageStyles : 
  public QObject,
  public IPlugin,
  public IMessageStyles
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageStyles);
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
  //IMessageStyles
  virtual IMessageStyle *styleById(const QString &AStyleId);
  virtual QList<QString> styles() const;
  virtual QList<QString> styleVariants(const QString &AStyleId) const;
  virtual QMap<QString, QVariant> styleInfo(const QString &AStyleId) const;
  virtual IMessageStyles::StyleSettings styleSettings(int AMessageType) const;
  virtual void setStyleSettings(int AMessageType, const IMessageStyles::StyleSettings &ASettings);
  virtual QString userAvatar(const Jid &AContactJid) const;
  virtual QString userName(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const;
  virtual QString userIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const;
  virtual QString userIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
  virtual QString messageTimeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const;
signals:
  virtual void styleCreated(IMessageStyle *AStyle) const;
  virtual void styleSettingsChanged(int AMessageType, const IMessageStyles::StyleSettings &ASettings) const;
protected:
  void updateAvailStyles();
protected slots:
  void onVCardChanged(const Jid &AContactJid);
private:
  ISettingsPlugin *FSettingsPlugin;
  IAvatars *FAvatars;
  IStatusIcons *FStatusIcons;
  IVCardPlugin *FVCardPlugin;
  IRosterPlugin *FRosterPlugin;
private:
  QMap<QString, QString> FStylePaths;
  QMap<QString, IMessageStyle *> FStyles;
  mutable QHash<Jid, QString> FStreamNames;
};

#endif // MESSAGESTYLES_H
