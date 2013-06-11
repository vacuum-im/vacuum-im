#ifndef ISTATISTICS_H
#define ISTATISTICS_H

#include <QUuid>
#include <QDateTime>

#define STATISTICS_UUID "{C9344821-9406-4089-A9D0-D6FD4919CF8F}"

// Measurement Protocol - https://developers.google.com/analytics/devguides/collection/protocol/v1/

struct IStatisticsHit {
	enum HitType {
		HitNone,
		HitView,
		HitEvent,
		HitTiming,
		HitException,
	};
	enum SessionControl {
		SessionNone,
		SessionStart,
		SessionEnd,
	};
	int type;
	int session;
	QString description;
	QDateTime timestamp;
	struct {
		QString title;
		QString location;
		QString host;
		QString path;
	} view;
	struct {
		QString category;
		QString action;
		QString label;
		qint64 value;
	} event;
	struct {
		QString category;
		QString variable;
		QString label;
		qint64 time;
	} timing;
	struct {
		QString errString;
		bool fatal;
	} exception;
	IStatisticsHit() {
		type = HitNone;
		session = SessionNone;
		event.value = -1;
		timing.time = -1;
		exception.fatal = false;
		timestamp = QDateTime::currentDateTime();
	}
};

class IStatistics
{
public:
	virtual QObject *instance() =0;
	virtual QUuid profileId() const =0;
	virtual bool isValidHit(const IStatisticsHit &AHit) const =0;
	virtual bool sendStatisticsHit(const IStatisticsHit &AHit) =0;
};

Q_DECLARE_INTERFACE(IStatistics,"Vacuum.Plugin.IStatistics/1.0");

#endif // ISTATISTICS_H
