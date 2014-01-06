#ifndef DEF_ROSTERINDEXKINDS_H
#define DEF_ROSTERINDEXKINDS_H

enum RosterIndexKind {
	RIK_ANY_KIND,
	RIK_ROOT,
	RIK_RECENT_ROOT,
	RIK_STREAM_ROOT,
	RIK_CONTACTS_ROOT,
	RIK_GROUP,
	RIK_GROUP_MUC,
	RIK_GROUP_BLANK,
	RIK_GROUP_NOT_IN_ROSTER,
	RIK_GROUP_MY_RESOURCES,
	RIK_GROUP_AGENTS,
	RIK_GROUP_ACCOUNTS,
	RIK_CONTACT,
	RIK_AGENT,
	RIK_MY_RESOURCE,
	RIK_RECENT_ITEM,
	RIK_MUC_ITEM,
	RIK_USER_KIND = 128
};

#endif //DEF_ROSTERINDEXKINDS_H