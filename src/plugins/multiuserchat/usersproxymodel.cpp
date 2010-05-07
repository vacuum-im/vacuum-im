#include "usersproxymodel.h"

UsersProxyModel::UsersProxyModel(IMultiUserChat *AMultiChat, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FMultiChat = AMultiChat;
}

UsersProxyModel::~UsersProxyModel()
{

}

bool UsersProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	static const QList<QString> roles = QList<QString>() << MUC_ROLE_MODERATOR << MUC_ROLE_PARTICIPANT << MUC_ROLE_VISITOR << MUC_ROLE_NONE;
	static const QList<QString> affiliations = QList<QString>() << MUC_AFFIL_OWNER << MUC_AFFIL_ADMIN << MUC_AFFIL_MEMBER << MUC_AFFIL_OUTCAST << MUC_AFFIL_NONE;

	IMultiUser *leftUser = FMultiChat->userByNick(ALeft.data(Qt::DisplayRole).toString());
	IMultiUser *rightUser = FMultiChat->userByNick(ARight.data(Qt::DisplayRole).toString());
	if (leftUser && rightUser)
	{
		int leftAffilIndex =  affiliations.indexOf(leftUser->data(MUDR_AFFILIATION).toString());
		int rightAffilIndex = affiliations.indexOf(rightUser->data(MUDR_AFFILIATION).toString());
		if (leftAffilIndex != rightAffilIndex)
			return leftAffilIndex < rightAffilIndex;

		int leftRoleIndex =  roles.indexOf(leftUser->data(MUDR_ROLE).toString());
		int rightRoleIndex = roles.indexOf(rightUser->data(MUDR_ROLE).toString());
		if (leftRoleIndex!=rightRoleIndex)
			return leftRoleIndex < rightRoleIndex;
	}
	return QString::localeAwareCompare(ALeft.data(Qt::DisplayRole).toString(),ARight.data(Qt::DisplayRole).toString())<0;
}
