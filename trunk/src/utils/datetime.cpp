#include "datetime.h"

#include <QRegExp>
#include <QStringList>

DateTimeData::DateTimeData(const QDateTime &AUTC, int ATZD)
{
	tzd = ATZD;
	utc = AUTC;
	utc.setTimeSpec(Qt::UTC);
}

DateTimeData::DateTimeData(const DateTimeData &AOther) : QSharedData(AOther)
{
	tzd = AOther.tzd;
	utc = AOther.utc;
}


DateTime::DateTime(const QString &AX85DateTime)
{
	d = new DateTimeData(utcFromX85(AX85DateTime),tzdFromX85(AX85DateTime));
}

DateTime::DateTime(const QDateTime &ADateTime, Qt::TimeSpec ASpec)
{
	if (ASpec == Qt::LocalTime)
	{
		QDateTime utc = ADateTime.toUTC();
		utc.setTimeSpec(Qt::LocalTime);
		d= new DateTimeData(utc,utc.secsTo(ADateTime));
	}
	else
	{
		d = new DateTimeData(ADateTime,0);
	}
}

DateTime::~DateTime()
{

}

bool DateTime::isNull() const
{
	return d->utc.isNull();
}

bool DateTime::isValid() const
{
	return d->utc.isValid();
}

int DateTime::timeZone() const
{
	return d->tzd;
}

void DateTime::setTimeZone(int ASecs)
{
	d->tzd = ASecs;
}

QDateTime DateTime::utcDateTime() const
{
	return d->utc;
}

void DateTime::setUTCDateTime(const QDateTime &AUTCDateTime)
{
	d->utc = AUTCDateTime;
	d->utc.setTimeSpec(Qt::UTC);
}

void DateTime::setDateTime(const QString &AX85DateTime)
{
	setUTCDateTime(utcFromX85(AX85DateTime));
	setTimeZone(tzdFromX85(AX85DateTime));
}

QDateTime DateTime::toUTC() const
{
	return d->utc;
}

QDateTime DateTime::toLocal() const
{
	return d->utc.toLocalTime();
}

QDateTime DateTime::toRemote() const
{
	QDateTime dateTime = d->utc;
	dateTime.setTimeSpec(Qt::LocalTime);
	return dateTime.addSecs(d->tzd);
}

QString DateTime::toX85Date() const
{
	QString x85 = d->utc.date().toString(Qt::ISODate);
	return x85;
}

QString DateTime::toX85Time(bool AMSec) const
{
	QString x85 = d->utc.time().toString(Qt::ISODate);
	if (AMSec)
		x85 += QString(".%1").arg(d->utc.time().msec(),3,10,QLatin1Char('0'));
	return x85;
}

QString DateTime::toX85TZD() const
{
	QString x85;
	if (d->tzd >= 0)
	{
		x85 += "+";
		x85 += QTime(0,0,0,0).addSecs(d->tzd).toString("hh:mm");
	}
	else if (d->tzd < 0)
	{
		x85 += "-";
		x85 += QTime(0,0,0,0).addSecs(-(d->tzd)).toString("hh:mm");
	}
	return x85;
}

QString DateTime::toX85UTC(bool AMSec) const
{
	return toX85Format(true,true,false,AMSec);
}

QString DateTime::toX85Full(bool AMSec) const
{
	return toX85Format(true,true,true,AMSec);
}

QString DateTime::toX85Format(bool ADate, bool ATime, bool ATZD, bool AMSec) const
{
	QString x85;
	if (ADate)
		x85 += toX85Date();
	if (ADate && ATime)
		x85 += "T";
	if (ATime)
		x85 += toX85Time(AMSec);
	if (ATZD)
		x85 += toX85TZD();
	else if (ATime)
		x85 += "Z";
	return x85;
}

QDateTime DateTime::utcFromX85(const QString &AX85DateTime)
{
	QDateTime utc;
	QRegExp utcRegExp("((\\d{4}-?\\d{2}-?\\d{2})?T?(\\d{2}:\\d{2}:\\d{2})?(\\.\\d{3})?)");
	if (utcRegExp.indexIn(AX85DateTime) > -1)
	{
		QString utcStr = utcRegExp.cap(1);
		utc = QDateTime::fromString(utcStr,Qt::ISODate);
		if (!utc.isValid())
		{
			QString format;
			bool hasTime = AX85DateTime.contains(':');
			bool hasMSec = AX85DateTime.contains('.');
			bool hasDate = !hasTime || AX85DateTime.contains('T');
			if (hasDate)
				format += "yyyyMMdd";
			if (hasDate && hasTime)
				format += "T";
			if (hasTime)
				format += "hh:mm:ss";
			if (hasMSec)
				format += ".zzz";
			utc = QDateTime::fromString(utcStr,format);
		}
	}
	utc.setTimeSpec(Qt::UTC);
	return utc;
}

int DateTime::tzdFromX85(const QString &AX85DateTime)
{
	int tzd = 0;
	QRegExp tzdRegExp("[+-](\\d{2}:\\d{2})");
	if (tzdRegExp.indexIn(AX85DateTime) > -1)
	{
		QTime time = QTime::fromString(tzdRegExp.cap(1),"hh:mm");
		tzd = AX85DateTime.contains('+') ? QTime(0,0,0,0).secsTo(time) : time.secsTo(QTime(0,0,0,0));
	}
	return tzd;
}


