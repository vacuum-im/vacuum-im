#pragma once
#include <QString>

class CrashHandlerPrivate;
class CrashHandler
{
public:
	static CrashHandler* instance();
	void init(const QString &path);
	void setReportCrashesToSystem(bool report);
	bool writeMinidump();

private:
	CrashHandler();
	~CrashHandler();
	Q_DISABLE_COPY(CrashHandler)
	CrashHandlerPrivate* d;
};
