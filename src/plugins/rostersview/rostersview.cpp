#include "rostersview.h"

#include <QCursor>
#include <QToolTip>
#include <QPainter>
#include <QDropEvent>
#include <QHelpEvent>
#include <QClipboard>
#include <QHeaderView>
#include <QResizeEvent>
#include <QApplication>
#include <QTextDocument>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QContextMenuEvent>

#define ADR_CLIPBOARD_DATA      Action::DR_Parametr1

RostersView::RostersView(QWidget *AParent) : QTreeView(AParent)
{
	FNotifyId = 1;
	FLabelIdCounter = 1;

	FRostersModel = NULL;

	FPressedPos = QPoint();
	FPressedLabel = RLID_NULL;
	FPressedIndex = QModelIndex();

	FBlinkShow = true;
	FBlinkTimer.setInterval(500);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimer()));

	header()->hide();
	header()->setStretchLastSection(false);

	setIndentation(4);
	setAutoScroll(true);
	setAcceptDrops(true);
	setRootIsDecorated(false);
	setSelectionMode(SingleSelection);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	FRosterIndexDelegate = new RosterIndexDelegate(this);
	setItemDelegate(FRosterIndexDelegate);

	FDragExpandTimer.setSingleShot(true);
	FDragExpandTimer.setInterval(500);
	connect(&FDragExpandTimer,SIGNAL(timeout()),SLOT(onDragExpandTimer()));

	setAcceptDrops(true);
	setDragEnabled(true);
	setDropIndicatorShown(true);

	connect(this,SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
	        SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
	connect(this,SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));
}

RostersView::~RostersView()
{
	removeLabels();
}

void RostersView::setRostersModel(IRostersModel *AModel)
{
	if (FRostersModel != AModel)
	{
		emit modelAboutToBeSeted(AModel);

		if (selectionModel())
			selectionModel()->clear();

		if (FRostersModel)
		{
			disconnect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),this,SLOT(onIndexInserted(IRosterIndex *)));
			disconnect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),this,SLOT(onIndexDestroyed(IRosterIndex *)));
			removeLabels();
		}

		FRostersModel = AModel;

		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),this,SLOT(onIndexInserted(IRosterIndex *)));
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)), SLOT(onIndexDestroyed(IRosterIndex *)));
		}

		if (FProxyModels.isEmpty())
		{
			emit viewModelAboutToBeChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
			QTreeView::setModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
			emit viewModelChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else
			FProxyModels.values().first()->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);

		emit modelSeted(FRostersModel);
	}
}

bool RostersView::repaintRosterIndex(IRosterIndex *AIndex)
{
	if (FRostersModel)
	{
		QModelIndex modelIndex = mapFromModel(FRostersModel->modelIndexByRosterIndex(AIndex));
		if (modelIndex.isValid())
		{
			QRect rect = visualRect(modelIndex).adjusted(1,1,-1,-1);
			if (!rect.isEmpty())
			{
				viewport()->repaint(rect);
				return true;
			}
		}
	}
	return false;
}

void RostersView::expandIndexParents(IRosterIndex *AIndex)
{
	QModelIndex index = FRostersModel->modelIndexByRosterIndex(AIndex);
	index = mapFromModel(index);
	expandIndexParents(index);
}

void RostersView::expandIndexParents(const QModelIndex &AIndex)
{
	QModelIndex index = AIndex;
	while (index.parent().isValid())
	{
		expand(index.parent());
		index = index.parent();
	}
}

void RostersView::insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder)
{
	if (AProxyModel && !FProxyModels.values().contains(AProxyModel))
	{
		emit proxyModelAboutToBeInserted(AProxyModel,AOrder);

		bool changeViewModel = FProxyModels.upperBound(AOrder) == FProxyModels.end();

		if (changeViewModel)
			emit viewModelAboutToBeChanged(AProxyModel);

		IRosterIndex *selectedIndex = FRostersModel!=NULL ? FRostersModel->rosterIndexByModelIndex(selectionModel()!=NULL ? mapToModel(selectionModel()->currentIndex()) : QModelIndex()) : NULL;
		if (selectionModel())
			selectionModel()->clear();

		FProxyModels.insert(AOrder,AProxyModel);
		QList<QAbstractProxyModel *> proxies = FProxyModels.values();
		int index = proxies.indexOf(AProxyModel);

		QAbstractProxyModel *before = proxies.value(index-1,NULL);
		QAbstractProxyModel *after = proxies.value(index+1,NULL);

		if (before)
		{
			AProxyModel->setSourceModel(before);
		}
		else
		{
			AProxyModel->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		if (after)
		{
			after->setSourceModel(NULL);  //костыли для QSortFilterProxyModel, аналогичные в removeProxyModel
			after->setSourceModel(AProxyModel);
		}
		else
		{
			QTreeView::setModel(AProxyModel);
		}

		if (selectedIndex)
			setCurrentIndex(mapFromModel(FRostersModel->modelIndexByRosterIndex(selectedIndex)));

		if (changeViewModel)
			emit viewModelChanged(model());

		emit proxyModelInserted(AProxyModel);
	}
}

QList<QAbstractProxyModel *> RostersView::proxyModels() const
{
	return FProxyModels.values();
}

void RostersView::removeProxyModel(QAbstractProxyModel *AProxyModel)
{
	if (FProxyModels.values().contains(AProxyModel))
	{
		emit proxyModelAboutToBeRemoved(AProxyModel);

		QList<QAbstractProxyModel *> proxies = FProxyModels.values();
		int index = proxies.indexOf(AProxyModel);

		QAbstractProxyModel *before = proxies.value(index-1,NULL);
		QAbstractProxyModel *after = proxies.value(index+1,NULL);

		bool changeViewModel = after==NULL;
		if (changeViewModel)
		{
			if (before!=NULL)
				emit viewModelAboutToBeChanged(before);
			else
				emit viewModelAboutToBeChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}

		IRosterIndex *selectedIndex = FRostersModel!=NULL ? FRostersModel->rosterIndexByModelIndex(selectionModel()!=NULL ? mapToModel(selectionModel()->currentIndex()) : QModelIndex()) : NULL;
		if (selectionModel())
			selectionModel()->clear();

		FProxyModels.remove(FProxyModels.key(AProxyModel),AProxyModel);

		if (after == NULL && before == NULL)
		{
			QTreeView::setModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else if (after == NULL)
		{
			QTreeView::setModel(before);
		}
		else if (before == NULL)
		{
			after->setSourceModel(NULL);
			after->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else
		{
			after->setSourceModel(NULL);
			after->setSourceModel(before);
		}

		AProxyModel->setSourceModel(NULL);

		if (selectedIndex)
			setCurrentIndex(mapFromModel(FRostersModel->modelIndexByRosterIndex(selectedIndex)));

		if (changeViewModel)
			emit viewModelChanged(model());

		emit proxyModelRemoved(AProxyModel);
	}
}

QModelIndex RostersView::mapToModel(const QModelIndex &AProxyIndex) const
{
	QModelIndex index = AProxyIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
		do
		{
			it--;
			index = it.value()->mapToSource(index);
		} while (it != FProxyModels.constBegin());
	}
	return index;
}

QModelIndex RostersView::mapFromModel(const QModelIndex &AModelIndex) const
{
	QModelIndex index = AModelIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
		while (it != FProxyModels.constEnd())
		{
			index = it.value()->mapFromSource(index);
			it++;
		}
	}
	return index;
}

QModelIndex RostersView::mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const
{
	QModelIndex index = AModelIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int,QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
		while (it!=FProxyModels.constEnd())
		{
			index = it.value()->mapFromSource(index);
			if (it.value() == AProxyModel)
				return index;
			it++;
		}
	}
	return index;
}

QModelIndex RostersView::mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const
{
	QModelIndex index = AProxyIndex;
	if (!FProxyModels.isEmpty())
	{
		bool doMap = false;
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
		do
		{
			it--;
			if (it.value() == AProxyModel)
				doMap = true;
			if (doMap)
				index = it.value()->mapToSource(index);
		} while (it != FProxyModels.constBegin());
	}
	return index;
}

int RostersView::createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags)
{
	int labelId = 0;
	if (ALabel.isValid())
	{
		labelId = FLabelIdCounter++;
		FIndexLabels.insert(labelId,ALabel);
		FIndexLabelOrders.insert(labelId,AOrder);
		FIndexLabelFlags.insert(labelId,AFlags);
		if (AFlags & IRostersView::LabelBlink)
			appendBlinkLabel(labelId);
	}
	return labelId;
}

void RostersView::updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags)
{
	if (ALabel.isValid() && FIndexLabels.contains(ALabelId) && FIndexLabels.value(ALabelId)!=ALabel)
	{
		FIndexLabels[ALabelId] = ALabel;
		FIndexLabelFlags[ALabelId] = AFlags;
		foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
		{
			QList<QVariant> ids = index->data(RDR_LABEL_ID).toList();
			QList<QVariant> labels = index->data(RDR_LABEL_VALUES).toList();
			QList<QVariant> flags = index->data(RDR_LABEL_FLAGS).toList();

			int i = 0;
			while (ids.at(i).toInt() != ALabelId)
				i++;

			labels[i] = ALabel;
			flags[i] = AFlags;
			if (AFlags & IRostersView::LabelBlink)
				appendBlinkLabel(ALabelId);
			else
				removeBlinkLabel(ALabelId);

			index->setData(RDR_LABEL_VALUES,labels);
			index->setData(RDR_LABEL_FLAGS,flags);
		}
	}
}

void RostersView::insertIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
	if (AIndex && FIndexLabels.contains(ALabelId) && !FIndexLabelIndexes.value(ALabelId).contains(AIndex))
	{
		QList<QVariant> ids = AIndex->data(RDR_LABEL_ID).toList();
		QList<QVariant> labels = AIndex->data(RDR_LABEL_VALUES).toList();
		QList<QVariant> orders = AIndex->data(RDR_LABEL_ORDERS).toList();
		QList<QVariant> flags = AIndex->data(RDR_LABEL_FLAGS).toList();

		int i = 0;
		int order = FIndexLabelOrders.value(ALabelId);
		while (i<orders.count() && orders.at(i).toInt() < order)
			i++;

		ids.insert(i,ALabelId);
		orders.insert(i,order);
		labels.insert(i,FIndexLabels.value(ALabelId));
		flags.insert(i,FIndexLabelFlags.value(ALabelId));
		FIndexLabelIndexes[ALabelId] += AIndex;

		if ((FIndexLabelFlags.value(ALabelId) & LabelExpandParents) > 0)
			expandIndexParents(AIndex);

		AIndex->setData(RDR_LABEL_ID,ids);
		AIndex->setData(RDR_LABEL_VALUES,labels);
		AIndex->setData(RDR_LABEL_FLAGS,flags);
		AIndex->setData(RDR_LABEL_ORDERS,orders);
	}
}

void RostersView::removeIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
	if (AIndex && FIndexLabels.contains(ALabelId) && FIndexLabelIndexes.value(ALabelId).contains(AIndex))
	{
		QList<QVariant> ids = AIndex->data(RDR_LABEL_ID).toList();
		QList<QVariant> labels = AIndex->data(RDR_LABEL_VALUES).toList();
		QList<QVariant> orders = AIndex->data(RDR_LABEL_ORDERS).toList();
		QList<QVariant> flags = AIndex->data(RDR_LABEL_FLAGS).toList();

		int i = 0;
		while (i<ids.count() && ids.at(i).toInt()!=ALabelId)
			i++;

		ids.removeAt(i);
		orders.removeAt(i);
		labels.removeAt(i);
		flags.removeAt(i);

		if (FIndexLabelIndexes.value(ALabelId).count() > 1)
			FIndexLabelIndexes[ALabelId] -= AIndex;
		else
			FIndexLabelIndexes.remove(ALabelId);

		AIndex->setData(RDR_LABEL_ORDERS,orders);
		AIndex->setData(RDR_LABEL_FLAGS,flags);
		AIndex->setData(RDR_LABEL_VALUES,labels);
		AIndex->setData(RDR_LABEL_ID,ids);
	}
}

void RostersView::destroyIndexLabel(int ALabelId)
{
	if (FIndexLabels.contains(ALabelId))
	{
		removeBlinkLabel(ALabelId);
		foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
			removeIndexLabel(ALabelId,index);
		FIndexLabels.remove(ALabelId);
		FIndexLabelOrders.remove(ALabelId);
		FIndexLabelFlags.remove(ALabelId);
		FIndexLabelIndexes.remove(ALabelId);
	}
}

int RostersView::labelAt(const QPoint &APoint, const QModelIndex &AIndex) const
{
	if (itemDelegate(AIndex) != FRosterIndexDelegate)
		return RLID_DISPLAY;

	return FRosterIndexDelegate->labelAt(APoint,indexOption(AIndex),AIndex);
}

QRect RostersView::labelRect(int ALabeld, const QModelIndex &AIndex) const
{
	if (itemDelegate(AIndex) != FRosterIndexDelegate)
		return QRect();

	return FRosterIndexDelegate->labelRect(ALabeld,indexOption(AIndex),AIndex);
}

int RostersView::appendNotify(QList<IRosterIndex *> AIndexes, int AOrder, const QIcon &AIcon, const QString &AToolTip, int AFlags)
{
	if (!AIndexes.isEmpty() && !AIcon.isNull())
	{
		int notifyId = FNotifyId++;
		NotifyItem notifyItem;
		notifyItem.notifyId = notifyId;
		notifyItem.order = AOrder;
		notifyItem.icon = AIcon;
		notifyItem.toolTip = AToolTip;
		notifyItem.flags = AFlags;
		notifyItem.indexes = AIndexes;
		FNotifyItems.insert(notifyId,notifyItem);

		foreach(IRosterIndex *index, AIndexes)
		{
			int labelId;
			QHash<int, int> &indexOrderLabel = FNotifyIndexOrderLabel[index];
			if (!indexOrderLabel.contains(AOrder))
			{
				labelId = createIndexLabel(AOrder,AIcon,AFlags);
				insertIndexLabel(labelId,index);
				indexOrderLabel.insert(AOrder,labelId);
			}
			else
			{
				labelId = indexOrderLabel.value(AOrder);
				updateIndexLabel(labelId,AIcon,AFlags);
				insertIndexLabel(labelId,index);
			}
			FNotifyLabelItems[labelId].prepend(notifyId);
		}
		return notifyId;
	}
	return 0;
}

QList<int> RostersView::indexNotifies(IRosterIndex *AIndex, int AOrder) const
{
	int labelId = FNotifyIndexOrderLabel.value(AIndex).value(AOrder,0);
	return FNotifyLabelItems.value(labelId);
}

void RostersView::updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, int AFlags)
{
	if (FNotifyItems.contains(ANotifyId))
	{
		NotifyItem &notifyItem = FNotifyItems[ANotifyId];
		notifyItem.icon = AIcon;
		notifyItem.toolTip = AToolTip;
		notifyItem.flags = AFlags;

		foreach(IRosterIndex *index, notifyItem.indexes)
		{
			int labelId = FNotifyIndexOrderLabel[index].value(notifyItem.order);
			if (FNotifyLabelItems[labelId].first() == ANotifyId)
				updateIndexLabel(labelId,AIcon,AFlags);
		}
	}
}

void RostersView::removeNotify(int ANotifyId)
{
	if (FNotifyItems.contains(ANotifyId))
	{
		NotifyItem &notifyItem = FNotifyItems[ANotifyId];
		foreach(IRosterIndex *index, notifyItem.indexes)
		{
			int labelId = FNotifyIndexOrderLabel[index].value(notifyItem.order);
			QList<int> &labelItems = FNotifyLabelItems[labelId];
			labelItems.removeAt(labelItems.indexOf(ANotifyId));
			if (!labelItems.isEmpty())
			{
				NotifyItem &firstNotifyItem = FNotifyItems[labelItems.first()];
				updateIndexLabel(labelId,firstNotifyItem.icon,firstNotifyItem.flags);
			}
			else
				removeIndexLabel(labelId,index);
		}
		FNotifyItems.remove(ANotifyId);
	}
}

void RostersView::insertClickHooker(int AOrder, IRostersClickHooker *AHooker)
{
	FClickHookers.insertMulti(AOrder,AHooker);
}

void RostersView::removeClickHooker(int AOrder, IRostersClickHooker *AHooker)
{
	FClickHookers.remove(AOrder,AHooker);
}

void RostersView::insertDragDropHandler(IRostersDragDropHandler *AHandler)
{
	if (!FDragDropHandlers.contains(AHandler))
	{
		FDragDropHandlers.append(AHandler);
		emit dragDropHandlerInserted(AHandler);
	}
}

void RostersView::removeDragDropHandler(IRostersDragDropHandler *AHandler)
{
	if (FDragDropHandlers.contains(AHandler))
	{
		FDragDropHandlers.removeAt(FDragDropHandlers.indexOf(AHandler));
		emit dragDropHandlerRemoved(AHandler);
	}
}

void RostersView::insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex)
{
	if (!AValue.isNull())
	{
		QString footerId = intId2StringId(AOrderAndId);
		QMap<QString,QVariant> footerMap = AIndex->data(RDR_FOOTER_TEXT).toMap();
		footerMap.insert(footerId, AValue);
		AIndex->setData(RDR_FOOTER_TEXT,footerMap);
	}
	else
		removeFooterText(AOrderAndId,AIndex);
}

void RostersView::removeFooterText(int AOrderAndId, IRosterIndex *AIndex)
{
	QString footerId = intId2StringId(AOrderAndId);
	QMap<QString,QVariant> footerMap = AIndex->data(RDR_FOOTER_TEXT).toMap();
	if (footerMap.contains(footerId))
	{
		footerMap.remove(footerId);
		if (!footerMap.isEmpty())
			AIndex->setData(RDR_FOOTER_TEXT,footerMap);
		else
			AIndex->setData(RDR_FOOTER_TEXT,QVariant());
	}
}

void RostersView::contextMenuForIndex(IRosterIndex *AIndex, int ALabelId, Menu *AMenu)
{
	if (AIndex!=NULL && AMenu!=NULL)
	{
		if (FNotifyLabelItems.contains(ALabelId))
			emit notifyContextMenu(AIndex,FNotifyLabelItems.value(ALabelId).first(),AMenu);
		else if (ALabelId != RLID_DISPLAY)
			emit labelContextMenu(AIndex,ALabelId,AMenu);
		else
			emit indexContextMenu(AIndex,AMenu);
	}
}

void RostersView::clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu)
{
	if (AIndex!=NULL && AMenu!=NULL)
	{
		if (!AIndex->data(RDR_JID).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Jabber ID"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_JID));
			action->setShortcutId(SCT_ROSTERVIEW_COPYJID);
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		if (!AIndex->data(RDR_STATUS).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Status"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_STATUS));
			action->setShortcutId(SCT_ROSTERVIEW_COPYSTATUS);
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		if (!AIndex->data(RDR_NAME).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Name"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_NAME));
			action->setShortcutId(SCT_ROSTERVIEW_COPYNAME);
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		emit indexClipboardMenu(AIndex, AMenu);
	}
}

void RostersView::updateStatusText(IRosterIndex *AIndex)
{
	const static QList<int> statusTypes = QList<int>() << RIT_STREAM_ROOT << RIT_CONTACT << RIT_AGENT;

	QList<IRosterIndex *> indexes;
	if (AIndex == NULL)
	{
		QMultiHash<int,QVariant> findData;
		foreach(int type, statusTypes)
			findData.insert(RDR_TYPE,type);
		IRosterIndex *streamRoot = FRostersModel!=NULL ? FRostersModel->rootIndex() : NULL;
		if (streamRoot)
		{
			indexes = streamRoot->findChild(findData,true);
			indexes.append(streamRoot);
		}
	}
	else if (statusTypes.contains(AIndex->type()))
	{
		indexes.append(AIndex);
	}

	bool show = Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool();
	foreach(IRosterIndex *index, indexes)
	{
		if (show)
			insertFooterText(FTO_ROSTERSVIEW_STATUS,RDR_STATUS,index);
		else
			removeFooterText(FTO_ROSTERSVIEW_STATUS,index);
	}
}

QStyleOptionViewItemV4 RostersView::indexOption(const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 option = viewOptions();
	option.initFrom(this);
	option.rect = visualRect(AIndex);
	option.showDecorationSelected |= selectionBehavior() & SelectRows;
	option.state |= isExpanded(AIndex) ? QStyle::State_Open : QStyle::State_None;
	if (hasFocus() && currentIndex() == AIndex)
		option.state |= QStyle::State_HasFocus;
	if (selectedIndexes().contains(AIndex))
		option.state |= QStyle::State_Selected;
	if ((AIndex.flags() & Qt::ItemIsEnabled) == 0)
		option.state &= ~QStyle::State_Enabled;
	if (indexAt(viewport()->mapFromGlobal(QCursor::pos())) == AIndex)
		option.state |= QStyle::State_MouseOver;
	if (wordWrap())
		option.features = QStyleOptionViewItemV2::WrapText;
	option.locale = locale();
	option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
	option.widget = this;
	return option;
}

void RostersView::appendBlinkLabel(int ALabelId)
{
	FBlinkLabels+=ALabelId;
	if (!FBlinkTimer.isActive())
		FBlinkTimer.start();
}

void RostersView::removeBlinkLabel(int ALabelId)
{
	FBlinkLabels-=ALabelId;
	if (FBlinkLabels.isEmpty() && FBlinkTimer.isActive())
		FBlinkTimer.stop();
}

QString RostersView::intId2StringId(int AIntId)
{
	return QString("%1").arg(AIntId,10,10,QLatin1Char('0'));
}

void RostersView::removeLabels()
{
	QList<int> labels = FIndexLabels.keys();
	foreach (int label, labels)
	{
		QSet<IRosterIndex *> indexes = FIndexLabelIndexes.value(label);
		foreach(IRosterIndex *index, indexes)
			removeIndexLabel(label,index);
	}
}

void RostersView::setDropIndicatorRect(const QRect &ARect)
{
	if (FDropIndicatorRect != ARect)
	{
		FDropIndicatorRect = ARect;
		viewport()->update();
	}
}

void RostersView::drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const
{
	Q_UNUSED(APainter);
	Q_UNUSED(ARect);
	Q_UNUSED(AIndex);
}

bool RostersView::viewportEvent(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent);
		QModelIndex viewIndex = indexAt(helpEvent->pos());
		if (viewIndex.isValid())
		{
			QMultiMap<int,QString> toolTipsMap;
			const int labelId = labelAt(helpEvent->pos(),viewIndex);

			QModelIndex modelIndex = mapToModel(viewIndex);
			IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());
			if (index != NULL)
			{
				emit labelToolTips(index,labelId,toolTipsMap);
				if (labelId!=RLID_DISPLAY && toolTipsMap.isEmpty())
					emit labelToolTips(index,RLID_DISPLAY,toolTipsMap);

				if (!toolTipsMap.isEmpty())
					QToolTip::showText(helpEvent->globalPos(),"<span>"+QStringList(toolTipsMap.values()).join("<p/>")+"</span>",this);

				return true;
			}
		}
	}
	return QTreeView::viewportEvent(AEvent);
}

void RostersView::resizeEvent(QResizeEvent *AEvent)
{
	if (model() && model()->columnCount()>0)
		header()->resizeSection(0,AEvent->size().width());
	QTreeView::resizeEvent(AEvent);
}

void RostersView::paintEvent(QPaintEvent *AEvent)
{
	QTreeView::paintEvent(AEvent);
	if (!FDropIndicatorRect.isNull())
	{
		QStyleOption option;
		option.init(this);
		option.rect = FDropIndicatorRect.adjusted(0,0,-1,-1);
		QPainter painter(viewport());
		style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &option, &painter, this);
	}
}

void RostersView::contextMenuEvent(QContextMenuEvent *AEvent)
{
	QModelIndex modelIndex = indexAt(AEvent->pos());
	if (modelIndex.isValid())
	{
		const int labelId = labelAt(AEvent->pos(),modelIndex);

		modelIndex = mapToModel(modelIndex);
		IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

		Menu *contextMenu = new Menu(this);
		contextMenu->setAttribute(Qt::WA_DeleteOnClose, true);

		contextMenuForIndex(index,labelId,contextMenu);
		if (labelId!=RLID_DISPLAY && contextMenu->isEmpty())
			contextMenuForIndex(index,RLID_DISPLAY,contextMenu);

		if (!contextMenu->isEmpty())
			contextMenu->popup(AEvent->globalPos());
		else
			delete contextMenu;
	}
}

void RostersView::mouseDoubleClickEvent(QMouseEvent *AEvent)
{
	bool accepted = false;
	if (viewport()->rect().contains(AEvent->pos()))
	{
		QModelIndex viewIndex = indexAt(AEvent->pos());
		if (viewIndex.isValid())
		{
			QModelIndex modelIndex = mapToModel(viewIndex);
			IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

			if (index != NULL)
			{
				const int labelId = labelAt(AEvent->pos(),viewIndex);

				if (!FNotifyLabelItems.contains(labelId))
				{
					emit labelDoubleClicked(index,labelId,accepted);

					QMultiMap<int,IRostersClickHooker *>::iterator it = FClickHookers.begin();
					while (!accepted && it!=FClickHookers.end())
					{
						accepted = it.value()->rosterIndexClicked(index,it.key());
						it++;
					}
				}
				else
					emit notifyActivated(index,FNotifyLabelItems.value(labelId).first());
			}
		}
	}
	if (!accepted)
		QTreeView::mouseDoubleClickEvent(AEvent);
}

void RostersView::mousePressEvent(QMouseEvent *AEvent)
{
	FStartDragFailed = false;
	FPressedPos = AEvent->pos();
	if (viewport()->rect().contains(FPressedPos))
	{
		FPressedIndex = indexAt(FPressedPos);
		if (FPressedIndex.isValid())
		{
			FPressedLabel = labelAt(AEvent->pos(),FPressedIndex);
			if (AEvent->button()==Qt::LeftButton && FPressedLabel==RLID_INDICATORBRANCH)
				setExpanded(FPressedIndex,!isExpanded(FPressedIndex));
		}
	}
	QTreeView::mousePressEvent(AEvent);
}

void RostersView::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (!FStartDragFailed && AEvent->buttons()!=Qt::NoButton && FPressedIndex.isValid() &&
	    (AEvent->pos()-FPressedPos).manhattanLength() > QApplication::startDragDistance())
	{
		QDrag *drag = new QDrag(this);
		drag->setMimeData(new QMimeData);

		Qt::DropActions actions = Qt::IgnoreAction;
		foreach(IRostersDragDropHandler *handler, FDragDropHandlers)
			actions |= handler->rosterDragStart(AEvent,FPressedIndex,drag);

		if (actions != Qt::IgnoreAction)
		{
			QAbstractItemDelegate *itemDeletage = itemDelegate(FPressedIndex);
			if (itemDeletage)
			{
				QStyleOptionViewItemV4 option = indexOption(FPressedIndex);
				QPoint indexPos = option.rect.topLeft();
				option.state &= ~QStyle::State_Selected;
				option.state &= ~QStyle::State_MouseOver;
				option.rect = QRect(QPoint(0,0),option.rect.size());
				QPixmap pixmap(option.rect.size());
				QPainter painter(&pixmap);
				painter.fillRect(option.rect,style()->standardPalette().color(QPalette::Normal,QPalette::Base));
				itemDeletage->paint(&painter,option,FPressedIndex);
				painter.drawRect(option.rect.adjusted(0,0,-1,-1));
				drag->setPixmap(pixmap);
				drag->setHotSpot(FPressedPos - indexPos);
			}

			QByteArray data;
			QDataStream stream(&data,QIODevice::WriteOnly);
			operator<<(stream,model()->itemData(FPressedIndex));
			drag->mimeData()->setData(DDT_ROSTERSVIEW_INDEX_DATA,data);

			setState(DraggingState);
			drag->exec(actions);
			setState(NoState);
		}
		else
		{
			FStartDragFailed = true;
		}
	}
	QTreeView::mouseMoveEvent(AEvent);
}

void RostersView::mouseReleaseEvent(QMouseEvent *AEvent)
{
	bool isClick = (FPressedPos-AEvent->pos()).manhattanLength() < QApplication::startDragDistance();
	if (isClick && AEvent->button()==Qt::LeftButton && viewport()->rect().contains(AEvent->pos()))
	{
		QModelIndex viewIndex = indexAt(AEvent->pos());
		const int labelId = viewIndex.isValid() ? labelAt(AEvent->pos(),viewIndex) : RLID_NULL;
		if (FPressedIndex.isValid() && FPressedIndex==viewIndex && FPressedLabel==labelId)
		{
			IRosterIndex *index = static_cast<IRosterIndex *>(mapToModel(viewIndex).internalPointer());
			if (index)
			{
				if (FNotifyLabelItems.contains(labelId))
					emit notifyActivated(index,FNotifyLabelItems.value(labelId).first());
				else
					emit labelClicked(index,labelId!=RLID_NULL ? labelId : RLID_DISPLAY);
			}
		}
	}

	FPressedPos = QPoint();
	FPressedLabel = RLID_NULL;
	FPressedIndex = QModelIndex();

	QTreeView::mouseReleaseEvent(AEvent);
}

void RostersView::dropEvent(QDropEvent *AEvent)
{
	Menu *dropMenu = new Menu(this);

	bool accepted = false;
	QModelIndex index = indexAt(AEvent->pos());
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		if (handler->rosterDropAction(AEvent,index,dropMenu))
			accepted = true;

	QAction *action= (AEvent->mouseButtons() & Qt::RightButton)>0 || dropMenu->defaultAction()==NULL ? dropMenu->exec(mapToGlobal(AEvent->pos())) : dropMenu->defaultAction();
	if (accepted && action)
	{
		action->trigger();
		AEvent->acceptProposedAction();
	}
	else
	{
		AEvent->ignore();
	}

	delete dropMenu;
	stopAutoScroll();
	setDropIndicatorRect(QRect());
}

void RostersView::dragEnterEvent(QDragEnterEvent *AEvent)
{
	FActiveDragHandlers.clear();
	foreach (IRostersDragDropHandler *handler, FDragDropHandlers)
		if (handler->rosterDragEnter(AEvent))
			FActiveDragHandlers.append(handler);

	if (!FActiveDragHandlers.isEmpty())
	{
		if (hasAutoScroll())
			startAutoScroll();
		AEvent->acceptProposedAction();
	}
	else
	{
		AEvent->ignore();
	}
}

void RostersView::dragMoveEvent(QDragMoveEvent *AEvent)
{
	QModelIndex index = indexAt(AEvent->pos());

	bool accepted = false;
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		if (handler->rosterDragMove(AEvent,index))
			accepted = true;

	if (accepted)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();

	if (!isExpanded(index))
		FDragExpandTimer.start();
	else
		FDragExpandTimer.stop();

	setDropIndicatorRect(visualRect(index));
}

void RostersView::dragLeaveEvent(QDragLeaveEvent *AEvent)
{
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		handler->rosterDragLeave(AEvent);
	stopAutoScroll();
	setDropIndicatorRect(QRect());
}

void RostersView::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	Menu *clipMenu = new Menu(AMenu);
	clipMenu->setTitle(tr("Copy to clipboard"));
	clipMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERVIEW_CLIPBOARD);
	clipboardMenuForIndex(AIndex, clipMenu);
	if (!clipMenu->isEmpty())
		AMenu->addAction(clipMenu->menuAction(), AG_RVCM_ROSTERSVIEW_CLIPBOARD, true);
	else
		delete clipMenu;
}

void RostersView::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
	if (ALabelId == RLID_DISPLAY)
	{
		QString name = AIndex->data(RDR_NAME).toString();
		if (!name.isEmpty())
			AToolTips.insert(RTTO_CONTACT_NAME, Qt::escape(name));

		QString jid = AIndex->data(RDR_JID).toString();
		if (!jid.isEmpty())
			AToolTips.insert(RTTO_CONTACT_JID, Qt::escape(jid));

		QString priority = AIndex->data(RDR_PRIORITY).toString();
		if (!priority.isEmpty())
			AToolTips.insert(RTTO_CONTACT_PRIORITY, tr("Priority: %1").arg(priority.toInt()));

		QString ask = AIndex->data(RDR_ASK).toString();
		QString subscription = AIndex->data(RDR_SUBSCRIBTION).toString();
		if (!subscription.isEmpty())
			AToolTips.insert(RTTO_CONTACT_SUBSCRIPTION, tr("Subscription: %1 %2").arg(Qt::escape(subscription)).arg(Qt::escape(ask)));

		QString status = AIndex->data(RDR_STATUS).toString();
		if (!status.isEmpty())
			AToolTips.insert(RTTO_CONTACT_STATUS, QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Status:")).arg(Qt::escape(status).replace("\n","<br>")));
	}
	else if (FNotifyLabelItems.contains(ALabelId))
	{
		NotifyItem &notifyItem = FNotifyItems[FNotifyLabelItems.value(ALabelId).first()];
		if (!notifyItem.toolTip.isEmpty())
			AToolTips.insert(RTTO_CONTACT_NOTIFY, notifyItem.toolTip);
	}
}

void RostersView::onCopyToClipboardActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
	}
}

void RostersView::onIndexInserted(IRosterIndex *AIndex)
{
	updateStatusText(AIndex);
}

void RostersView::onIndexDestroyed(IRosterIndex *AIndex)
{
	if (FNotifyIndexOrderLabel.contains(AIndex))
	{
		QList<int> labels = FNotifyIndexOrderLabel[AIndex].values();
		foreach(int labelId, labels)
		{
			QList<int> notifyIds = FNotifyLabelItems.take(labelId);
			foreach(int notifyId, notifyIds)
			{
				NotifyItem &notifyItem = FNotifyItems[notifyId];
				if (notifyItem.indexes.count() == 1)
				{
					emit notifyRemovedByIndex(AIndex,notifyId);
					removeNotify(notifyId);
				}
				else
					notifyItem.indexes.removeAt(notifyItem.indexes.indexOf(AIndex));
			}
			destroyIndexLabel(labelId);
		}
		FNotifyIndexOrderLabel.remove(AIndex);
	}

	QHash<int, QSet<IRosterIndex *> >::iterator it = FIndexLabelIndexes.begin();
	while (it!=FIndexLabelIndexes.end())
	{
		if (it.value().contains(AIndex))
			it.value() -= AIndex;

		if (it.value().isEmpty())
			it = FIndexLabelIndexes.erase(it);
		else
			it++;
	}
}

void RostersView::onBlinkTimer()
{
	FBlinkShow = !FBlinkShow;
	FRosterIndexDelegate->setShowBlinkLabels(FBlinkShow);
	foreach(int labelId,FBlinkLabels)
		foreach(IRosterIndex *index, FIndexLabelIndexes.value(labelId))
			repaintRosterIndex(index);
}

void RostersView::onDragExpandTimer()
{
	QModelIndex index = indexAt(FDropIndicatorRect.center());
	setExpanded(index,true);
}

void RostersView::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	QModelIndex index = currentIndex();
	if (AId==SCT_ROSTERVIEW_COPYJID && AWidget==this)
	{
		if (!index.data(RDR_JID).toString().isEmpty())
			QApplication::clipboard()->setText(index.data(RDR_JID).toString());
	}
	else if (AId==SCT_ROSTERVIEW_COPYNAME && AWidget==this)
	{
		if (!index.data(RDR_NAME).toString().isEmpty())
			QApplication::clipboard()->setText(index.data(RDR_NAME).toString());
	}
	else if (AId==SCT_ROSTERVIEW_COPYSTATUS && AWidget==this)
	{
		if (!index.data(RDR_STATUS).toString().isEmpty())
			QApplication::clipboard()->setText(index.data(RDR_STATUS).toString());
	}
}
