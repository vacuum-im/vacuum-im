#include "discoitemsmodel.h"

DiscoItemsModel::DiscoItemsModel(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QObject *AParent) : QAbstractItemModel(AParent)
{
	FDiscovery = ADiscovery;
	FStreamJid = AStreamJid;
	FEnableDiscoCache = false;
	FRootIndex = new DiscoItemIndex;
	FRootIndex->infoFetched = true;
	FRootIndex->itemsFetched = true;

	IPlugin *plugin = FDiscovery->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
	connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsReceived(const IDiscoItems &)));
}

DiscoItemsModel::~DiscoItemsModel()
{
	delete FRootIndex;
}

QModelIndex DiscoItemsModel::parent(const QModelIndex &AIndex) const
{
	if (AIndex.isValid())
	{
		DiscoItemIndex *index = itemIndex(AIndex);
		if (index && index->parent)
			return modelIndex(index->parent,AIndex.column());
	}
	return QModelIndex();
}

QModelIndex DiscoItemsModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
	return modelIndex(itemIndex(AParent)->childs.value(ARow),AColumn);
}

bool DiscoItemsModel::canFetchMore(const QModelIndex &AParent) const
{
	DiscoItemIndex *index = itemIndex(AParent);
	return !index->infoFetched || !index->itemsFetched;
}

void DiscoItemsModel::fetchMore(const QModelIndex &AParent)
{
	fetchIndex(AParent);
}

bool DiscoItemsModel::hasChildren(const QModelIndex &AParent) const
{
	DiscoItemIndex *index = itemIndex(AParent);
	return !index->itemsFetched || !index->childs.isEmpty();
}

int DiscoItemsModel::rowCount(const QModelIndex &AParent) const
{
	return itemIndex(AParent)->childs.count();
}

int DiscoItemsModel::columnCount(const QModelIndex &AParent) const
{
	Q_UNUSED(AParent);
	return COL__COUNT;
}

Qt::ItemFlags DiscoItemsModel::flags(const QModelIndex &AIndex) const
{
	Q_UNUSED(AIndex);
	return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

QVariant DiscoItemsModel::data(const QModelIndex &AIndex, int ARole) const
{
	DiscoItemIndex *index = itemIndex(AIndex);
	switch (ARole)
	{
	case Qt::DisplayRole:
		switch (AIndex.column())
		{
		case COL_NAME:
			return !index->itemName.isEmpty() ? index->itemName : index->itemJid.full();
		case COL_JID:
			return index->itemJid.full();
		case COL_NODE:
			return index->itemNode;
		}
		break;
	case Qt::DecorationRole:
		switch (AIndex.column())
		{
		case COL_NAME:
			return index->icon;
		}
		break;
	case Qt::ToolTipRole:
		switch (AIndex.column())
		{
		case COL_NAME:
			return index->toolTip;
		case COL_JID:
			return index->itemJid.full();
		case COL_NODE:
			return index->itemNode;
		}
		break;
	case DIDR_NAME:
		return index->itemName;
	case DIDR_STREAM_JID:
		return FStreamJid.full();
	case DIDR_JID:
		return index->itemJid.full();
	case DIDR_NODE:
		return index->itemNode;
	}
	return QVariant();
}

QVariant DiscoItemsModel::headerData(int ASection, Qt::Orientation AOrientation, int ARole) const
{
	if (ARole == Qt::DisplayRole && AOrientation==Qt::Horizontal)
	{
		switch (ASection)
		{
		case COL_NAME:
			return tr("Name");
		case COL_JID:
			return tr("Jid");
		case COL_NODE:
			return tr("Node");
		}
	}
	return QAbstractItemModel::headerData(ASection,AOrientation,ARole);
}

void DiscoItemsModel::fetchIndex(const QModelIndex &AIndex, bool AInfo, bool AItems)
{
	DiscoItemIndex *index = itemIndex(AIndex);
	if (index && (AInfo || AItems))
	{
		if (AInfo && !index->infoFetched)
		{
			if (isDiscoCacheEnabled() && FDiscovery->hasDiscoInfo(FStreamJid, index->itemJid,index->itemNode))
				onDiscoInfoReceived(FDiscovery->discoInfo(FStreamJid,index->itemJid,index->itemNode));
			else
				FDiscovery->requestDiscoInfo(FStreamJid,index->itemJid,index->itemNode);
		}
		if (AItems && !index->itemsFetched)
		{
			FDiscovery->requestDiscoItems(FStreamJid,index->itemJid,index->itemNode);
		}
		index->icon = FDiscovery->serviceIcon(FStreamJid,index->itemJid,index->itemNode);
		emit dataChanged(AIndex,AIndex);
	}
}

void DiscoItemsModel::loadIndex(const QModelIndex &AIndex, bool AInfo, bool AItems)
{
	DiscoItemIndex *index = itemIndex(AIndex);
	if (index)
	{
		if (AInfo)
			FDiscovery->requestDiscoInfo(FStreamJid, index->itemJid,index->itemNode);
		if (AItems)
			FDiscovery->requestDiscoItems(FStreamJid,index->itemJid,index->itemNode);
		index->icon = FDiscovery->serviceIcon(FStreamJid,index->itemJid,index->itemNode);
		emit dataChanged(AIndex,AIndex);
	}
}

bool DiscoItemsModel::isDiscoCacheEnabled() const
{
	return FEnableDiscoCache;
}

void DiscoItemsModel::setDiscoCacheEnabled(bool AEnabled)
{
	FEnableDiscoCache = AEnabled;
}

void DiscoItemsModel::appendTopLevelItem(const Jid &AItemJid, const QString &AItemNode)
{
	if (findIndex(AItemJid,AItemNode,FRootIndex,false).isEmpty())
	{
		DiscoItemIndex *index = new DiscoItemIndex;
		index->itemJid = AItemJid;
		index->itemNode = AItemNode;
		appendChildren(FRootIndex, QList<DiscoItemIndex*>()<<index);
		fetchMore(modelIndex(index,0));
	}
}

void DiscoItemsModel::removeTopLevelItem(int AIndex)
{
	if (AIndex < FRootIndex->childs.count())
		removeChildren(FRootIndex,QList<DiscoItemIndex*>()<<FRootIndex->childs.at(AIndex));
}

DiscoItemIndex *DiscoItemsModel::itemIndex(const QModelIndex &AIndex) const
{
	return AIndex.isValid() ? reinterpret_cast<DiscoItemIndex *>(AIndex.internalPointer()) : FRootIndex;
}

QModelIndex DiscoItemsModel::modelIndex(DiscoItemIndex *AIndex, int AColumn) const
{
	return AIndex!=NULL && AIndex!=FRootIndex ? createIndex(AIndex->parent->childs.indexOf(AIndex),AColumn,AIndex) : QModelIndex();
}

QList<DiscoItemIndex *> DiscoItemsModel::findIndex(const Jid &AItemJid, const QString &AItemNode, DiscoItemIndex *AParent, bool ARecursive) const
{
	QList<DiscoItemIndex *> indexList;
	DiscoItemIndex *parentIndex = AParent!=NULL ? AParent : FRootIndex;
	for (int i=0; i<parentIndex->childs.count(); i++)
	{
		DiscoItemIndex *curIndex = parentIndex->childs.at(i);
		if (curIndex->itemJid == AItemJid && curIndex->itemNode==AItemNode)
			indexList.append(curIndex);
		if (ARecursive)
			indexList += findIndex(AItemJid,AItemNode,curIndex,ARecursive);
	}
	return indexList;
}

QString DiscoItemsModel::itemToolTip(const IDiscoInfo &ADiscoInfo) const
{
	QString toolTip;
	if (ADiscoInfo.error.code < 0)
	{
		if (!ADiscoInfo.identity.isEmpty())
		{
			toolTip+=tr("<li><b>Identity:</b></li>");
			foreach(IDiscoIdentity identity, ADiscoInfo.identity)
				toolTip+=tr("<li>%1 (Category: '%2'; Type: '%3')</li>").arg(identity.name).arg(identity.category).arg(identity.type);
		}

		if (!ADiscoInfo.features.isEmpty())
		{
			QStringList features = ADiscoInfo.features;
			qSort(features);
			toolTip+=tr("<li><b>Features:</b></li>");
			foreach(QString feature, features)
			{
				IDiscoFeature dfeature = FDiscovery->discoFeature(feature);
				toolTip+=QString("<li>%1</li>").arg(dfeature.name.isEmpty() ? feature : dfeature.name);
			}
		}

		if (FDataForms)
		{
			for (int iform=0; iform<ADiscoInfo.extensions.count(); iform++)
			{
				IDataForm form = FDataForms->localizeForm(ADiscoInfo.extensions.at(iform));
				toolTip += QString("<li><b>%1:</b></li>").arg(!form.title.isEmpty() ? form.title : FDataForms->fieldValue("FORM_TYPE",form.fields).toString());
				foreach(IDataField field, form.fields)
				{
					if (field.var != "FORM_TYPE")
					{
						QStringList values;
						if (field.value.type() == QVariant::StringList)
							values = field.value.toStringList();
						else if (field.value.type() == QVariant::Bool)
							values +=(field.value.toBool() ? tr("true") : tr("false"));
						else
							values += field.value.toString();
						toolTip += QString("<li>%1: %2</li>").arg(!field.label.isEmpty() ? field.label : field.var).arg(values.join("; "));
					}
				}
			}
		}
	}
	else
		toolTip+=tr("Error %1: %2").arg(ADiscoInfo.error.code).arg(ADiscoInfo.error.message);
	return toolTip;
}

void DiscoItemsModel::updateDiscoInfo(DiscoItemIndex *AIndex, const IDiscoInfo &ADiscoInfo)
{
	if (AIndex->itemName.isEmpty())
	{
		for (int i=0; i<ADiscoInfo.identity.count();i++)
		{
			if (!ADiscoInfo.identity.at(i).name.isEmpty())
			{
				AIndex->itemName = ADiscoInfo.identity.at(i).name;
				break;
			}
		}
	}
	AIndex->toolTip = itemToolTip(ADiscoInfo);
	AIndex->icon = FDiscovery->serviceIcon(FStreamJid,AIndex->itemJid,AIndex->itemNode);
}

void DiscoItemsModel::appendChildren(DiscoItemIndex *AParent, QList<DiscoItemIndex *> AChilds)
{
	if (AParent && !AChilds.isEmpty())
	{
		QList<DiscoItemIndex *> childs;
		foreach(DiscoItemIndex *index, AChilds)
		{
			QList<DiscoItemIndex*> indexList = findIndex(index->itemJid,index->itemNode,AParent,false);
			if (indexList.isEmpty())
				childs.append(index);
			else if (!indexList.contains(index))
				delete index;
		}

		if (!childs.isEmpty())
		{
			emit beginInsertRows(modelIndex(AParent,0), AParent->childs.count(), AParent->childs.count()+childs.count()-1);
			foreach(DiscoItemIndex *index, childs)
			{
				index->parent = AParent;
				AParent->childs.append(index);
			}
			emit endInsertRows();
		}
	}
}

void DiscoItemsModel::removeChildren(DiscoItemIndex *AParent, QList<DiscoItemIndex *> AChilds)
{
	if (AParent && !AChilds.isEmpty())
	{
		QList<int> rows;
		foreach(DiscoItemIndex *index, AChilds)
		{
			int row = AParent->childs.indexOf(index);
			if (row >= 0)
				rows.append(row);
		}
		qSort(rows);

		int firstRow = -1;
		int lastRow = -1;
		while (!rows.isEmpty())
		{
			if (firstRow < 0)
			{
				firstRow = rows.takeLast();
				lastRow = firstRow;
			}
			if (!rows.isEmpty() && firstRow-1==rows.last())
			{
				firstRow = rows.takeLast();
			}
			if (rows.isEmpty() || firstRow-1!=rows.last())
			{
				emit beginRemoveRows(modelIndex(AParent,0),firstRow,lastRow);
				while (lastRow >= firstRow)
				{
					lastRow--;
					delete AParent->childs.takeAt(firstRow);
				}
				emit endRemoveRows();
				firstRow = -1;
			}
		}
	}
}

void DiscoItemsModel::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.streamJid == FStreamJid)
	{
		QList<DiscoItemIndex *> indexList = findIndex(ADiscoInfo.contactJid,ADiscoInfo.node);
		foreach(DiscoItemIndex *index, indexList)
		{
			index->infoFetched = true;
			updateDiscoInfo(index,ADiscoInfo);
			emit dataChanged(modelIndex(index,0),modelIndex(index,COL__COUNT-1));
		}
	}
}

void DiscoItemsModel::onDiscoItemsReceived(const IDiscoItems &ADiscoItems)
{
	if (ADiscoItems.streamJid == FStreamJid)
	{
		QList<DiscoItemIndex *> indexList = findIndex(ADiscoItems.contactJid,ADiscoItems.node);
		foreach(DiscoItemIndex *parentIndex, indexList)
		{
			parentIndex->itemsFetched = true;
			QList<DiscoItemIndex*> appendList;
			QList<DiscoItemIndex*> updateList;
			foreach(IDiscoItem item, ADiscoItems.items)
			{
				DiscoItemIndex *index = findIndex(item.itemJid,item.node,parentIndex,false).value(0);
				if (index == NULL)
				{
					index = new DiscoItemIndex;
					index->itemJid = item.itemJid;
					index->itemNode = item.node;
					appendList.append(index);
				}
				else
				{
					updateList.append(index);
				}
				if (!item.name.isEmpty())
				{
					index->itemName = item.name;
				}
			}

			QList<DiscoItemIndex *> removeList = (parentIndex->childs.toSet()-appendList.toSet()-updateList.toSet()).toList();
			removeChildren(parentIndex,removeList);

			QList<DiscoItemIndex *> loadList;
			foreach(DiscoItemIndex *newIndex, appendList)
			{
				if (isDiscoCacheEnabled() && FDiscovery->hasDiscoInfo(FStreamJid,newIndex->itemJid,newIndex->itemNode))
					updateDiscoInfo(newIndex,FDiscovery->discoInfo(FStreamJid,newIndex->itemJid,newIndex->itemName));
				else
					loadList.append(newIndex);
			}
			appendChildren(parentIndex,appendList);

			foreach (DiscoItemIndex *index,loadList)
			{
				if (loadList.count()<=MAX_ITEMS_FOR_REQUEST)
					FDiscovery->requestDiscoInfo(FStreamJid,index->itemJid,index->itemNode);
				index->icon = FDiscovery->serviceIcon(FStreamJid,index->itemJid,index->itemNode);
			}

			if (!updateList.isEmpty())
				emit dataChanged(modelIndex(parentIndex->childs.first(),0),modelIndex(parentIndex->childs.last(),COL__COUNT-1));
			emit dataChanged(modelIndex(parentIndex,0),modelIndex(parentIndex,0));
		}
	}
}
