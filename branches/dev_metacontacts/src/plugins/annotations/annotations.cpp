#include "annotations.h"

#include <QClipboard>
#include <QApplication>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/resources.h>
#include <utils/advanceditemdelegate.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/logger.h>

#define PST_ANNOTATIONS       "storage"
#define PSN_ANNOTATIONS       "storage:rosternotes"

#define ADR_STREAMJID         Action::DR_StreamJid
#define ADR_CONTACTJID        Action::DR_Parametr1
#define ADR_CLIPBOARD_DATA    Action::DR_Parametr2

static const QList<int> AnnotationRosterKinds = QList<int>() << RIK_CONTACT << RIK_AGENT << RIK_MUC_ITEM << RIK_METACONTACT_ITEM;

Annotations::Annotations()
{
	FPrivateStorage = NULL;
	FRosterSearch = NULL;
	FRosterPlugin = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;

	FSaveTimer.setInterval(0);
	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveAnnotationsTimerTimeout()));
}

Annotations::~Annotations()
{

}

void Annotations::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Annotations");
	APluginInfo->description = tr("Allows to add comments to the contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool Annotations::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataSaved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorageClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
				SLOT(onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterSearch").value(0,NULL);
	if (plugin)
	{
		FRosterSearch = qobject_cast<IRosterSearch *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FPrivateStorage!=NULL;
}

bool Annotations::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_EDITANNOTATION, tr("Edit annotation"), QKeySequence::UnknownKey, Shortcuts::WidgetShortcut);

	if (FRostersViewPlugin)
	{
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_EDITANNOTATION, FRostersViewPlugin->rostersView()->instance());
	}
	if (FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_ANNOTATIONS,this);
	}
	if (FRosterSearch)
	{
		FRosterSearch->insertSearchField(RDR_ANNOTATIONS,tr("Annotation"));
	}
	return true;
}

QList<int> Annotations::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_ANNOTATIONS)
		return QList<int>() << RDR_ANNOTATIONS;
	return QList<int>();
}

QVariant Annotations::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	Q_UNUSED(ARole);
	if (AOrder == RDHO_ANNOTATIONS)
	{
		if (AnnotationRosterKinds.contains(AIndex->kind()))
		{
			QString note = annotation(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_PREP_BARE_JID).toString());
			return !note.isEmpty() ? QVariant(note) : QVariant();
		}
	}
	return QVariant();
}

bool Annotations::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(ARole);
	if (AOrder == RDHO_ANNOTATIONS)
	{
		if (AnnotationRosterKinds.contains(AIndex->kind()))
		{
			setAnnotation(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_PREP_BARE_JID).toString(),AValue.toString());
			return true;
		}
	}
	return false;
}

bool Annotations::isEnabled(const Jid &AStreamJid) const
{
	return FAnnotations.contains(AStreamJid);
}

QList<Jid> Annotations::annotations(const Jid &AStreamJid) const
{
	return FAnnotations.value(AStreamJid).keys();
}

QString Annotations::annotation(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FAnnotations.value(AStreamJid).value(AContactJid.bare()).note;
}

QDateTime Annotations::annotationCreateDate(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FAnnotations.value(AStreamJid).value(AContactJid.bare()).created.toLocal();
}

QDateTime Annotations::annotationModifyDate(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FAnnotations.value(AStreamJid).value(AContactJid.bare()).modified.toLocal();
}

bool Annotations::setAnnotation(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANote)
{
	if (isEnabled(AStreamJid))
	{
		if (!ANote.isEmpty())
		{
			Annotation &item = FAnnotations[AStreamJid][AContactJid.bare()];
			item.modified = DateTime(QDateTime::currentDateTime());
			if (!item.created.isValid())
				item.created = item.modified;
			item.note = ANote;
		}
		else
		{
			FAnnotations[AStreamJid].remove(AContactJid.bare());
		}

		updateDataHolder(AStreamJid,QList<Jid>()<<AContactJid);
		emit annotationModified(AStreamJid,AContactJid);

		FSavePendingStreams += AStreamJid;
		FSaveTimer.start();
		return true;
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed to set annotation to=%1: Annotations is not enabled").arg(AContactJid.bare()));
	}
	return false;
}

QDialog *Annotations::showAnnotationDialog(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (isEnabled(AStreamJid))
	{
		EditNoteDialog *dialog = FEditDialogs.value(AStreamJid).value(AContactJid);
		if (!dialog)
		{
			dialog = new EditNoteDialog(this,AStreamJid,AContactJid);
			FEditDialogs[AStreamJid].insert(AContactJid,dialog);
			connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onEditNoteDialogDestroyed()));
		}
		WidgetManager::showActivateRaiseWindow(dialog);
		return dialog;
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to open annotation dialog: Annotations is not enabled");
	}
	return NULL;
}

bool Annotations::loadAnnotations(const Jid &AStreamJid)
{
	if (FPrivateStorage)
	{
		QString id = FPrivateStorage->loadData(AStreamJid,PST_ANNOTATIONS,PSN_ANNOTATIONS);
		if (!id.isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,QString("Annotations load request sent, id=%1").arg(id));
			FLoadRequests.insert(id,AStreamJid);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send load annotations request");
		}
	}
	return false;
}

bool Annotations::saveAnnotations(const Jid &AStreamJid)
{
	if (isEnabled(AStreamJid))
	{
		QDomDocument doc;
		QDomElement storage = doc.appendChild(doc.createElementNS(PSN_ANNOTATIONS,PST_ANNOTATIONS)).toElement();

		const QMap<Jid, Annotation> &items = FAnnotations.value(AStreamJid);
		QMap<Jid, Annotation>::const_iterator it = items.constBegin();
		while (it != items.constEnd())
		{
			QDomElement elem = storage.appendChild(doc.createElement("note")).toElement();
			elem.setAttribute("jid",it.key().bare());
			elem.setAttribute("cdate",it.value().created.toX85UTC());
			elem.setAttribute("mdate",it.value().modified.toX85UTC());
			elem.appendChild(doc.createTextNode(it.value().note));
			++it;
		}

		QString id = FPrivateStorage->saveData(AStreamJid,doc.documentElement());
		if (!id.isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,QString("Save annotations request sent, id=%1").arg(id));
			FSaveRequests.insert(id,AStreamJid);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send save annotations request");
		}
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to save annotations: Annotations is not ready");
	}
	return false;
}

void Annotations::updateDataHolder(const Jid &AStreamJid, const QList<Jid> &AContactJids)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot && !AContactJids.isEmpty())
	{
		QMultiMap<int,QVariant> findData;
		foreach(const Jid &contactJid, AContactJids)
			findData.insertMulti(RDR_PREP_BARE_JID,contactJid.pBare());
		findData.insertMulti(RDR_STREAM_JID,AStreamJid.pFull());

		QList<IRosterIndex *> indexes = sroot->findChilds(findData,true);
		foreach (IRosterIndex *index, indexes)
			emit rosterDataChanged(index,RDR_ANNOTATIONS);
	}
}

void Annotations::onSaveAnnotationsTimerTimeout()
{
	foreach(const Jid &streamJid, FSavePendingStreams)
		saveAnnotations(streamJid);
	FSavePendingStreams.clear();
}

void Annotations::onPrivateStorageOpened(const Jid &AStreamJid)
{
	loadAnnotations(AStreamJid);
}

void Annotations::onPrivateDataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AElement);
	if (FSaveRequests.contains(AId))
	{
		LOG_STRM_INFO(AStreamJid,QString("Annotations saved, id=%1").arg(AId));
		FSaveRequests.remove(AId);
		emit annotationsSaved(AStreamJid);
	}
}

void Annotations::onPrivateDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	if (FLoadRequests.contains(AId))
	{
		LOG_STRM_INFO(AStreamJid,QString("Annotations loaded, id=%1").arg(AId));
		FLoadRequests.remove(AId);

		QMap<Jid, Annotation> &items = FAnnotations[AStreamJid];
		items.clear();

		QDomElement elem = AElement.firstChildElement("note");
		while (!elem.isNull())
		{
			Jid itemJid = elem.attribute("jid");
			if (itemJid.isValid() && !elem.text().isEmpty())
			{
				Annotation item;
				item.created = elem.attribute("cdate");
				item.modified = elem.attribute("mdate");
				item.note = elem.text();
				items.insert(itemJid.bare(),item);
			}
			elem= elem.nextSiblingElement("note");
		}
		emit annotationsLoaded(AStreamJid);
		updateDataHolder(AStreamJid,annotations(AStreamJid));
	}
}

void Annotations::onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (isEnabled(AStreamJid) && ATagName==PST_ANNOTATIONS && ANamespace==PSN_ANNOTATIONS)
		loadAnnotations(AStreamJid);
}

void Annotations::onPrivateStorageClosed(const Jid &AStreamJid)
{
	QList<Jid> curAnnotations = annotations(AStreamJid);

	qDeleteAll(FEditDialogs.take(AStreamJid));
	FAnnotations.remove(AStreamJid);

	updateDataHolder(AStreamJid,curAnnotations);
}

void Annotations::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.subscription==SUBSCRIPTION_REMOVE && isEnabled(ARoster->streamJid()))
	{
		if (!annotation(ARoster->streamJid(),AItem.itemJid).isEmpty())
			setAnnotation(ARoster->streamJid(),AItem.itemJid,QString::null);
	}
}

void Annotations::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersViewPlugin && AWidget==FRostersViewPlugin->rostersView()->instance() && !FRostersViewPlugin->rostersView()->hasMultiSelection())
	{
		if (AId == SCT_ROSTERVIEW_EDITANNOTATION)
		{
			IRosterIndex *index = !FRostersViewPlugin->rostersView()->hasMultiSelection() ? FRostersViewPlugin->rostersView()->selectedRosterIndexes().value(0) : NULL;
			if (index!=NULL && AnnotationRosterKinds.contains(index->data(RDR_KIND).toInt()))
				showAnnotationDialog(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString());
		}
	}
}

void Annotations::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		Jid streamJid = index->data(RDR_STREAM_JID).toString();
		Jid contactJid = index->data(RDR_PREP_BARE_JID).toString();
		if (AnnotationRosterKinds.contains(index->kind()) && isEnabled(streamJid) && contactJid.isValid())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Annotation"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_ANNOTATIONS);
			action->setData(ADR_STREAMJID,streamJid.full());
			action->setData(ADR_CONTACTJID,contactJid.bare());
			action->setShortcutId(SCT_ROSTERVIEW_EDITANNOTATION);
			connect(action,SIGNAL(triggered(bool)),SLOT(onEditNoteActionTriggered(bool)));
			AMenu->addAction(action,AG_RVCM_ANNOTATIONS,true);
		}
	}
}

void Annotations::onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		foreach(IRosterIndex *index, AIndexes)
		{
			QString note = index->data(RDR_ANNOTATIONS).toString();
			if (!note.isEmpty())
			{
				Action *noteAction = new Action(AMenu);
				noteAction->setText(TextManager::getElidedString(note,Qt::ElideRight,50));
				noteAction->setData(ADR_CLIPBOARD_DATA, note);
				connect(noteAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
				AMenu->addAction(noteAction, AG_RVCBM_ANNOTATION, true);
			}
		}
	}
}

void Annotations::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AnnotationRosterKinds.contains(AIndex->kind()))
	{
		QString note = AIndex->data(RDR_ANNOTATIONS).toString();
		if (!note.isEmpty())
			AToolTips.insert(RTTO_ANNOTATIONS, tr("<b>Annotation:</b>")+"<br>"+Qt::escape(note).replace("\n","<br>"));
	}
}

void Annotations::onCopyToClipboardActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
}

void Annotations::onEditNoteActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		showAnnotationDialog(action->data(ADR_STREAMJID).toString(),action->data(ADR_CONTACTJID).toString());
}

void Annotations::onEditNoteDialogDestroyed()
{
	EditNoteDialog *dialog = qobject_cast<EditNoteDialog *>(sender());
	if (dialog)
		FEditDialogs[dialog->streamJid()].remove(dialog->contactJid());
}

Q_EXPORT_PLUGIN2(plg_annotations, Annotations)
