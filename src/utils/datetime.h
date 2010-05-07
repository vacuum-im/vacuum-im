#ifndef DATETIME_H
#define DATETIME_H

#include <QDateTime>
#include <QSharedData>
#include "utilsexport.h"

class DateTimeData :
			public QSharedData
{
public:
	DateTimeData(const QDateTime &AUTC, int ATZD);
	DateTimeData(const DateTimeData &AOther);
public:
	int tzd;
	QDateTime utc;
};

class UTILS_EXPORT DateTime
{
public:
	DateTime(const QString &AX85DateTime);
	DateTime(const QDateTime &ADateTime=QDateTime(), Qt::TimeSpec ASpec=Qt::LocalTime);
	~DateTime();
	bool isNull() const;
	bool isValid() const;
	int timeZone() const;
	void setTimeZone(int ASecs);
	QDateTime utcDateTime() const;
	void setUTCDateTime(const QDateTime &AUTCDateTime);
	void setDateTime(const QString &AX85DateTime);
	QDateTime toUTC() const;
	QDateTime toLocal() const;
	QDateTime toRemote() const;
	QString toX85Date() const;
	QString toX85Time(bool AMSec = false) const;
	QString toX85TZD() const;
	QString toX85UTC(bool AMSec = false) const;
	QString toX85Full(bool AMSec = false) const;
	QString toX85Format(bool ADate, bool ATime, bool ATZD, bool AMSec = false) const;
public:
	static QDateTime utcFromX85(const QString &AX85DateTime);
	static int tzdFromX85(const QString &AX85DateTime);
private:
	QSharedDataPointer<DateTimeData> d;
};

#endif // DATETIME_H
