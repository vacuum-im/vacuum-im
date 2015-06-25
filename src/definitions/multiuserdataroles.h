#ifndef DEF_MULTIUSERDATAROLES_H
#define DEF_MULTIUSERDATAROLES_H

enum MultiUserDataRoles {
	MUDR_ANY_ROLE    = -1, // AdvancedItemModel::AnyRole
	MUDR_ALL_ROLES   = -2, // AdvancedItemModel::AllRoles
	MUDR_KIND        = 32, // Qt::UserRole
	// User Roles
	MUDR_STREAM_JID,
	MUDR_USER_JID,
	MUDR_REAL_JID,
	MUDR_NICK,
	MUDR_ROLE,
	MUDR_AFFILIATION,
	MUDR_PRESENCE,
	// View Roles
	MUDR_AVATAR_IMAGE,
	MUDR_PRESENCE_SHOW,
	MUDR_PRESENCE_STATUS,
	// View Delegate Roles
	MUDR_LABEL_ITEMS,
};

#endif // DEF_MULTIUSERDATAROLES_H
