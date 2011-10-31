#include "datetime.h"

#include <QRegExp>
#include <QStringList>

DateTimeData::DateTimeData(const QDateTime &ADT, int ATZD)
{
	tzd = ATZD;
	dt = ADT;
	dt.setTimeSpec(Qt::LocalTime);
}

DateTimeData::DateTimeData(const DateTimeData &AOther) : QSharedData(AOther)
{
	tzd = AOther.tzd;
	dt = AOther.dt;
}


DateTime::DateTime(const QString &AX85DateTime)
{
	d = new DateTimeData(dtFromX85(AX85DateTime),tzdFromX85(AX85DateTime));
}

DateTime::DateTime(const QDateTime &ADateTime)
{
	if (ADateTime.timeSpec() == Qt::LocalTime)
	{
		QDateTime utc = ADateTime.toUTC();
		utc.setTimeSpec(Qt::LocalTime);
		d = new DateTimeData(ADateTime,utc.secsTo(ADateTime));
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
	return d->dt.isNull();
}

bool DateTime::isValid() const
{
	return d->dt.isValid();
}

int DateTime::timeZone() const
{
	return d->tzd;
}

void DateTime::setTimeZone(int ASecs)
{
	d->tzd = ASecs;
}

QDateTime DateTime::dateTime() const
{
	return d->dt;
}

void DateTime::setDateTime(const QDateTime &ADateTime)
{
	d->dt = ADateTime;
	d->dt.setTimeSpec(Qt::LocalTime);
}

QDateTime DateTime::toUTC() const
{
	QDateTime utc = d->dt;
	utc.setTimeSpec(Qt::UTC);
	return utc.addSecs(-(d->tzd));
}

QDateTime DateTime::toLocal() const
{
	return toUTC().toLocalTime();
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

QString DateTime::toX85Date() const
{
	QString x85 = d->dt.date().toString(Qt::ISODate);
	return x85;
}

QString DateTime::toX85Time() const
{
	QString x85 = d->dt.time().toString(Qt::ISODate);
	int msecs = d->dt.time().msec();
	if (msecs > 0)
		x85 += QString(".%1").arg(msecs,3,10,QLatin1Char('0'));
	return x85;
}

QString DateTime::toX85DateTime() const
{
	return toX85Format(true,true,true);
}

QString DateTime::toX85UTC() const
{
	DateTime utc = toUTC();
	return utc.toX85Format(true,true,false);
}

QString DateTime::toX85Format(bool ADate, bool ATime, bool ATZD) const
{
	QString x85;
	if (ADate)
		x85 += toX85Date();
	if (ADate && ATime)
		x85 += "T";
	if (ATime)
		x85 += toX85Time();
	if (ATZD)
		x85 += toX85TZD();
	else if (ATime)
		x85 += "Z";
	return x85;
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

QDateTime DateTime::dtFromX85(const QString &AX85DateTime)
{
	QDateTime dt;
	QRegExp dtRegExp("((\\d{4}-?\\d{2}-?\\d{2})?T?(\\d{2}:\\d{2}:\\d{2})?(\\.\\d{3})?)");
	if (dtRegExp.indexIn(AX85DateTime) > -1)
	{
		QString dtStr = dtRegExp.cap(1);
		dt = QDateTime::fromString(dtStr,Qt::ISODate);
		if (!dt.isValid())
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
			dt = QDateTime::fromString(dtStr,format);
		}
	}
	return dt;
}
