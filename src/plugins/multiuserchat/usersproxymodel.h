#ifndef USERSPROXYMODEL_H
#define USERSPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <definations/multiuserdataroles.h>
#include <interfaces/imultiuserchat.h>

class UsersProxyModel :
			public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	UsersProxyModel(IMultiUserChat *AMultiChat, QObject *AParent);
	~UsersProxyModel();
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
private:
	IMultiUserChat *FMultiChat;
};

#endif // USERSPROXYMODEL_H
