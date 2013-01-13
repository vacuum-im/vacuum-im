#ifndef IBIRTHDAYREMINDER_H
#define IBIRTHDAYREMINDER_H

#include <QDate>
#include <QImage>
#include <utils/jid.h>

#define BIRTHDAYREMINDER_UUID "{3F41AF10-AB69-499a-B628-F5C4E6756BC7}"

class IBirthdayReminder
{
public:
	virtual QObject *instance() =0;
	virtual QDate contactBithday(const Jid &AContactJid) const =0;
	virtual int contactBithdayDaysLeft(const Jid &AContactJid) const =0;
};

Q_DECLARE_INTERFACE(IBirthdayReminder,"Vacuum.Plugin.IBirthdayReminer/1.0")

#endif // IBIRTHDAYREMINDER_H
