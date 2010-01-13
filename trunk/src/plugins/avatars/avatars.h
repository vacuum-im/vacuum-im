#ifndef AVATARS_H
#define AVATARS_H

#include <QDir>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/stanzahandlerorders.h>
#include <definations/rosterlabelorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterdataholderorders.h>
#include <definations/rostertooltiporders.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ivcard.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/isettings.h>
#include <utils/iconstorage.h>
#include "rosteroptionswidget.h"

class Avatars : 
  public QObject,
  public IPlugin,
  public IAvatars,
  public IStanzaHandler,
  public IStanzaRequestOwner,
  public IRosterIndexDataHolder,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IAvatars IStanzaHandler IRosterIndexDataHolder IStanzaRequestOwner IOptionsHolder);
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
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AId);
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
  virtual QString avatarHash(const Jid &AContactJid) const;
  virtual QImage avatarImage(const Jid &AContactJid) const;
  virtual bool setAvatar(const Jid &AStreamJid, const QImage &AImage, const char *AFormat = NULL);
  virtual QString setCustomPictire(const Jid &AContactJid, const QString &AImageFile);
  virtual bool avatarsVisible() const;
  virtual void setAvatarsVisible(bool AVisible);
  virtual bool showEmptyAvatars() const;
  virtual void setShowEmptyAvatars(bool AShow);
signals:
  void avatarChanged(const Jid &AContactJid);
  void avatarsVisibleChanged(bool AVisible);
  void showEmptyAvatarsChanged(bool AShow);
  //IRosterIndexDataHolder
  void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
  //IOptionsHolder
  void optionsAccepted();
  void optionsRejected();
protected:
  QByteArray loadAvatarFromVCard(const Jid &AContactJid) const;
  void updatePresence(const Jid &AStreamJid) const;
  void updateDataHolder(const Jid &AContactJid = Jid());
  bool updateVCardAvatar(const Jid &AContactJid, const QString &AHash);
  bool updateIqAvatar(const Jid &AContactJid, const QString &AHash);
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
  void onIconStorageChanged();
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IVCardPlugin *FVCardPlugin;
  IPresencePlugin *FPresencePlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  QMap<Jid, int> FSHIPresenceIn;
  QMap<Jid, int> FSHIPresenceOut;
  QMap<Jid, QString> FVCardAvatars;
private:
  QMap<Jid, int> FSHIIqAvatarIn;
  QMap<Jid, QString> FIqAvatars;
  QMap<QString, Jid> FIqAvatarRequests;
private:
  QMap<Jid, QString> FCustomPictures;
private:
  QSize FAvatarSize;
  bool FAvatarsVisible;
  bool FShowEmptyAvatars;
private:
  int FRosterLabelId;
  QDir FAvatarsDir;
  QImage FEmptyAvatar;
  QMap<Jid, QString> FStreamAvatars;
  mutable QHash<QString, QImage> FAvatarImages;
};

#endif // AVATARS_H
