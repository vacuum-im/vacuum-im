#ifndef ANNOTATIONS_H
#define ANNOTATIONS_H

#include <QTimer>
#include <QDomDocument>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/resources.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iannotations.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersearch.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/datetime.h>
#include <utils/shortcuts.h>
#include <utils/textmanager.h>
#include <utils/widgetmanager.h>
#include <utils/advanceditemdelegate.h>
#include "editnotedialog.h"

struct Annotation {
	DateTime created;
	DateTime modified;
	QString note;
};

class Annotations :
	public QObject,
	public IPlugin,
	public IAnnotations,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAnnotations IRosterDataHolder);
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
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IAnnotations
	virtual bool isEnabled(const Jid &AStreamJid) const;
	virtual QList<Jid> annotations(const Jid &AStreamJid) const;
	virtual QString annotation(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QDateTime annotationCreateDate(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QDateTime annotationModifyDate(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool setAnnotation(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANote);
	virtual QDialog *showAnnotationDialog(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void annotationsLoaded(const Jid &AStreamJid);
	void annotationsSaved(const Jid &AStreamJid);
	void annotationModified(const Jid &AStreamJid, const Jid &AContactJid);
	// IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	bool loadAnnotations(const Jid &AStreamJid);
	bool saveAnnotations(const Jid &AStreamJid);
	void updateDataHolder(const Jid &AStreamJid, const QList<Jid> &AContactJids);
protected slots:
	void onSaveAnnotationsTimerTimeout();
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateDataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageClosed(const Jid &AStreamJid);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRosterIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onCopyToClipboardActionTriggered(bool);
	void onEditNoteActionTriggered(bool);
	void onEditNoteDialogDestroyed();
private:
	IPrivateStorage *FPrivateStorage;
	IRosterSearch *FRosterSearch;
	IRosterPlugin *FRosterPlugin;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QTimer FSaveTimer;
	QSet<Jid> FSavePendingStreams;
	QMap<QString, Jid> FLoadRequests;
	QMap<QString, Jid> FSaveRequests;
	QMap<Jid, QMap<Jid, Annotation> > FAnnotations;
	QMap<Jid, QMap<Jid, EditNoteDialog *> > FEditDialogs;
};

#endif // ANNOTATIONS_H
