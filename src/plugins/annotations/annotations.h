#ifndef ANNOTATIONS_H
#define ANNOTATIONS_H

#include <QDomDocument>
#include "../../definations/rosterindextyperole.h"
#include "../../definations/actiongroups.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/tooltiporders.h"
#include "../../definations/menuicons.h"
#include "../../definations/resources.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iannotations.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/iprivatestorage.h"
#include "../../interfaces/irostersview.h"
#include "../../utils/datetime.h"
#include "editnotedialog.h"

struct Annotation {
  DateTime created;
  DateTime modified;
  QString note;
};

class Annotations : 
  public QObject,
  public IPlugin,
  public IAnnotations
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IAnnotations);
public:
  Annotations();
  ~Annotations();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return ANNOTATIONS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IAnnotations
  virtual bool isEnabled(const Jid &AStreamJid) const;
  virtual QList<Jid> annotations(const Jid &AStreamJid) const;
  virtual QString annotation(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QDateTime annotationCreateDate(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QDateTime annotationModifyDate(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual void setAnnotation(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANote);
  virtual bool loadAnnotations(const Jid &AStreamJid);
  virtual bool saveAnnotations(const Jid &AStreamJid);
signals:
  virtual void annotationsLoaded(const Jid &AStreamJid);
  virtual void annotationsSaved(const Jid &AStreamJid);
  virtual void annotationsError(const Jid &AStreamJid, const QString &AError);
  virtual void annotationModified(const Jid &AStreamJid, const Jid &AContactJid);
protected slots:
  void onPrivateStorageOpened(const Jid &AStreamJid);
  void onPrivateDataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onPrivateDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onPrivateDataError(const QString &AId, const QString &AError);
  void onPrivateStorageClosed(const Jid &AStreamJid);
  void onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  void onEditNoteActionTriggered(bool);
  void onEditNoteDialogDestroyed();
private:
  IPrivateStorage *FPrivateStorage;
  IRosterPlugin *FRosterPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  QHash<Jid, QString> FLoadRequests;
  QHash<Jid, QString> FSaveRequests;
  QHash<Jid, QHash<Jid, Annotation> > FAnnotations;
  QHash<Jid, QHash<Jid, EditNoteDialog *> > FEditDialogs;
};

#endif // ANNOTATIONS_H
