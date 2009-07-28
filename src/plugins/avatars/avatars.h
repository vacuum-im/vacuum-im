#ifndef AVATARS_H
#define AVATARS_H

#include <QDir>
#include "../../definations/namespaces.h"
#include "../../definations/actiongroups.h"
#include "../../definations/stanzahandlerpriority.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterdataholderorders.h"
#include "../../definations/rostertooltiporders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iavatars.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ivcard.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "rosteroptionswidget.h"

class Avatars : 
  public QObject,
  public IPlugin,
  public IAvatars,
  public IStanzaHandler,
  public IIqStanzaOwner,
  public IRosterIndexDataHolder,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IAvatars IStanzaHandler IRosterIndexDataHolder IIqStanzaOwner IOptionsHolder);
public:
  Avatars();
  ~Avatars();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return AVATARTS_UUID; }
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
  //IRosterIndexDataHolder
  virtual int order() const;
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IAvatars
  virtual QString avatarFileName(const QString &AHash) const;
  virtual bool hasAvatar(const QString &AHash) const;
  virtual QImage loadAvatar(const QString &AHash) const;
  virtual QString saveAvatar(const QByteArray &AImageData) const;
  virtual QString saveAvatar(const QImage &AImage, const char *AFormat = NULL) const;
  //Contacts Avatars
  virtual QString avatarHash(const Jid &AContactJid) const;
  virtual QImage avatarImage(const Jid &AContactJid) const;
  virtual QSize avatarSize() const { return FAvatarSize; }
  virtual void setAvatarSize(const QSize &ASize);
  virtual bool setAvatar(const Jid &AStreamJid, const QImage &AImage, const char *AFormat = NULL);
  virtual QString setCustomPictire(const Jid &AContactJid, const QString &AImageFile);
  //Options
  virtual bool checkOption(IAvatars::Option AOption) const;
  virtual void setOption(IAvatars::Option AOption, bool AValue);
signals:
  virtual void optionChanged(IAvatars::Option AOption, bool AValue);
  virtual void avatarChanged(const Jid &AContactJid);
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  QByteArray loadAvatarFromVCard(const Jid &AContactJid) const;
  void updatePresence(const Jid &AStreamJid) const;
  void updateDataHolder(const Jid &AContactJid = Jid());
  bool updateVCardAvatar(const Jid &AContactJid, const QString &AHash);
  bool updateIqAvatar(const Jid &AContactJid, const QString &AHash);
  QImage convertToGray(const QImage &AImage) const;
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onVCardChanged(const Jid &AContactJid);
  void onRosterIndexInserted(IRosterIndex *AIndex);
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  void onSetAvatarByAction(bool);
  void onClearAvatarByAction(bool);
  void onSettingsOpened();
  void onSettingsClosed();
  void onUpdateOptions();
private:
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IVCardPlugin *FVCardPlugin;
  IPresencePlugin *FPresencePlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  QHash<Jid, int> FSHIPresenceIn;
  QHash<Jid, int> FSHIPresenceOut;
  QHash<Jid, QString> FVCardAvatars;
private:
  QHash<Jid, int> FSHIIqAvatarIn;
  QHash<Jid, QString> FIqAvatars;
  QHash<QString,Jid> FIqAvatarRequests;
private:
  QHash<Jid, QString> FCustomPictures;
private:
  int FOptions;
  int FCurOptions;
private:
  int FRosterLabelId;
  QSize FAvatarSize;
  QDir FAvatarsDir;
  QImage FEmptyAvatar;
  QHash<Jid, QString> FStreamAvatars;
  mutable QHash<QString, QImage> FAvatarImages;
};

#endif // AVATARS_H
