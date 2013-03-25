#ifndef DATAHOLDER_H
#define DATAHOLDER_H

#include <interfaces/irostersmodel.h>

class DataHolder : 
	public QObject,
	public AdvancedItemDataHolder
{
	Q_OBJECT;
public:
	DataHolder(IRosterDataHolder *ADataHolder, IRostersModel *AModel);
	~DataHolder();
	virtual QList<int> advancedItemDataRoles(int AOrder) const;
	virtual QVariant advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const;
	virtual bool setAdvancedItemData(int AOrder, const QVariant &AValue, QStandardItem *AItem, int ARole);
protected slots:
	void onRosterDataChanged(IRosterIndex *AIndex, int ARole);
private:
	IRostersModel *FModel;
	IRosterDataHolder *FDataHolder;
};

#endif // DATAHOLDER_H
