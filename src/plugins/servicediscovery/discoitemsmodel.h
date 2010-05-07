#ifndef DISCOITEMSMODEL_H
#define DISCOITEMSMODEL_H

#include <QIcon>
#include <QList>
#include <QHash>
#include <QAbstractItemModel>
#include <definations/discoitemdataroles.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/idataforms.h>

#define MAX_ITEMS_FOR_REQUEST   20

struct DiscoItemIndex
{
	DiscoItemIndex() {
		parent = NULL;
		infoFetched = false;
		itemsFetched = false;
	}
	~DiscoItemIndex() {
		qDeleteAll(childs);
	}
	Jid itemJid;
	QString itemNode;
	QString itemName;
	QIcon icon;
	QString toolTip;
	bool infoFetched;
	bool itemsFetched;
	DiscoItemIndex *parent;
	QList<DiscoItemIndex *> childs;
};

class DiscoItemsModel :
			public QAbstractItemModel
{
	Q_OBJECT;
public:
	enum ModelColumns {
		COL_NAME,
		COL_JID,
		COL_NODE,
		COL__COUNT
	};
public:
	DiscoItemsModel(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QObject *AParent);
	~DiscoItemsModel();
	//QAbstractItemModel
	virtual QModelIndex parent(const QModelIndex &AIndex) const;
	virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
	virtual bool canFetchMore (const QModelIndex &AParent) const;
	virtual void fetchMore (const QModelIndex &AParent);
	virtual bool hasChildren(const QModelIndex &AParent) const;
	virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
	virtual Qt::ItemFlags flags(const QModelIndex &AIndex) const;
	virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
	virtual QVariant headerData(int ASection, Qt::Orientation AOrientation, int ARole = Qt::DisplayRole) const;
	//DiscoItemsModel
	void fetchIndex(const QModelIndex &AIndex, bool AInfo = true, bool AItems = true);
	void loadIndex(const QModelIndex &AIndex, bool AInfo = true, bool AItems = true);
	bool isDiscoCacheEnabled() const;
	void setDiscoCacheEnabled(bool AEnabled);
	void appendTopLevelItem(const Jid &AItemJid, const QString &AItemNode);
	void removeTopLevelItem(int AIndex);
protected:
	DiscoItemIndex *itemIndex(const QModelIndex &AIndex) const;
	QModelIndex modelIndex(DiscoItemIndex *AIndex, int AColumn) const;
	QList<DiscoItemIndex *> findIndex(const Jid &AItemJid, const QString &AItemNode, DiscoItemIndex *AParent=NULL, bool ARecursive=true) const;
	QString itemToolTip(const IDiscoInfo &ADiscoInfo) const;
	void updateDiscoInfo(DiscoItemIndex *AIndex, const IDiscoInfo &ADiscoInfo);
	void appendChildren(DiscoItemIndex *AParent, QList<DiscoItemIndex*> AChilds);
	void removeChildren(DiscoItemIndex *AParent, QList<DiscoItemIndex*> AChilds);
protected slots:
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onDiscoItemsReceived(const IDiscoItems &ADiscoItems);
private:
	IDataForms *FDataForms;
	IServiceDiscovery *FDiscovery;
private:
	Jid FStreamJid;
	bool FEnableDiscoCache;
	DiscoItemIndex *FRootIndex;
};

#endif // DISCOITEMSMODEL_H
