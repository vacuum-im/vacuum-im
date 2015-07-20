#include "multiuser.h"

#include <definitions/multiuserdataroles.h>
#include <utils/logger.h>

MultiUser::MultiUser(const Jid &AStreamJid, const Jid &AUserJid, const Jid &ARealJid, QObject *AParent) : QObject(AParent)
{
	FStreamJid = AStreamJid;
	FUserJid = AUserJid;
	FRealJid = ARealJid;
	FRole = MUC_ROLE_NONE;
	FAffiliation = MUC_AFFIL_NONE;

	LOG_STRM_DEBUG(FStreamJid,QString("User created, user=%1").arg(FUserJid.full()));
}

MultiUser::~MultiUser()
{
	LOG_STRM_DEBUG(FStreamJid,QString("User destroyed, user=%1").arg(FUserJid.full()));
}

Jid MultiUser::streamJid() const
{
	return FStreamJid;
}

Jid MultiUser::roomJid() const
{
	return FUserJid.bare();
}

Jid MultiUser::userJid() const
{
	return FUserJid;
}

Jid MultiUser::realJid() const
{
	return FRealJid;
}

QString MultiUser::nick() const
{
	return FUserJid.resource();
}

QString MultiUser::role() const
{
	return FRole;
}

QString MultiUser::affiliation() const
{
	return FAffiliation;
}

IPresenceItem MultiUser::presence() const
{
	return FPresence;
}

void MultiUser::setNick(const QString &ANick)
{
	if (FUserJid.resource() != ANick)
	{
		LOG_STRM_DEBUG(FStreamJid,QString("User nick changed to=%1, user=%2").arg(ANick,FUserJid.full()));

		QVariant before = FUserJid.resource();
		FUserJid.setResource(ANick);

		emit changed(MUDR_NICK,before);
	}
}

void MultiUser::setRole(const QString &ARole)
{
	if (FRole != ARole)
	{
		LOG_STRM_DEBUG(FStreamJid,QString("User role changed from=%1 to=%2, user=%3").arg(FRole,ARole,FUserJid.full()));

		QVariant before = FRole;
		FRole = ARole;

		emit changed(MUDR_ROLE,before);
	}
}

void MultiUser::setAffiliation(const QString &AAffiliation)
{
	if (FAffiliation != AAffiliation)
	{
		LOG_STRM_DEBUG(FStreamJid,QString("User affiliation changed from=%1 to=%2, user=%3").arg(FAffiliation,AAffiliation,FUserJid.full()));

		QVariant before = FAffiliation;
		FAffiliation = AAffiliation;
		
		emit changed(MUDR_AFFILIATION,before);
	}
}

void MultiUser::setPresence(const IPresenceItem &APresence)
{
	if (FPresence != APresence)
	{
		LOG_STRM_DEBUG(FStreamJid,QString("User presence changed from=%1 to=%2, user=%3").arg(FPresence.show).arg(APresence.show).arg(FUserJid.full()));

		FPresence = APresence;

		emit changed(MUDR_PRESENCE,QVariant());
	}
}
