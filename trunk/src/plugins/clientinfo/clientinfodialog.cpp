#include "clientinfodialog.h"

ClientInfoDialog::ClientInfoDialog(IClientInfo *AClientInfo, const Jid &AStreamJid, const Jid &AContactJid,
                                   const QString &AContactName, int AInfoTypes, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Client info - %1").arg(AContactName));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_CLIENTINFO,0,0,"windowIcon");

	FClientInfo = AClientInfo;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FContactName = AContactName;
	FInfoTypes = AInfoTypes;

	connect(FClientInfo->instance(),SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));
	connect(FClientInfo->instance(),SIGNAL(lastActivityChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));
	connect(FClientInfo->instance(),SIGNAL(entityTimeChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));

	setInfoTypes(AInfoTypes);
}

ClientInfoDialog::~ClientInfoDialog()
{
	emit clientInfoDialogClosed(FContactJid);
}

void ClientInfoDialog::setInfoTypes(int AInfoTypes)
{
	FInfoTypes = AInfoTypes;
	if ((FInfoTypes & IClientInfo::SoftwareVersion)>0)
		FClientInfo->requestSoftwareInfo(FStreamJid,FContactJid);
	if ((FInfoTypes & IClientInfo::LastActivity)>0)
		FClientInfo->requestLastActivity(FStreamJid,FContactJid);
	if ((FInfoTypes & IClientInfo::EntityTime)>0)
		FClientInfo->requestEntityTime(FStreamJid,FContactJid);
	updateText();
}

void ClientInfoDialog::updateText()
{
	QString itemMask = "%1 %2<br>";
	QString html = QString("<b>%1</b> (%2)<br><br>").arg(Qt::escape(FContactName)).arg(FContactJid.hFull());

	//Software Info
	if ((FInfoTypes & IClientInfo::SoftwareVersion)>0)
	{
		html += "<b>" + tr("Software Version") + "</b><br>";
		if (FClientInfo->hasSoftwareInfo(FContactJid))
		{
			html += itemMask.arg(tr("Name:")).arg(Qt::escape(FClientInfo->softwareName(FContactJid)));
			if (!FClientInfo->softwareVersion(FContactJid).isEmpty())
				html += itemMask.arg(tr("Version:")).arg(Qt::escape(FClientInfo->softwareVersion(FContactJid)));
			if (!FClientInfo->softwareOs(FContactJid).isEmpty())
				html += itemMask.arg(tr("OS:")).arg(Qt::escape(FClientInfo->softwareOs(FContactJid)));
		}
		else if (FClientInfo->softwareStatus(FContactJid) == IClientInfo::SoftwareError)
		{
			html += itemMask.arg(tr("Error:")).arg(FClientInfo->softwareName(FContactJid));
		}
		else
		{
			html += tr("Waiting response...") + "<br>";
		}
		html += "<br>";
	}

	//Last Activity
	if ((FInfoTypes & IClientInfo::LastActivity)>0)
	{
		if (FContactJid.node().isEmpty())
			html += "<b>" + tr("Service Uptime") + "</b>";
		else if (FContactJid.resource().isEmpty())
			html += "<b>" + tr("Last Activity") + "</b>";
		else
			html += "<b>" + tr("Idle Time") + "</b>";
		html += "<br>";

		if (FClientInfo->hasLastActivity(FContactJid))
		{
			if (FContactJid.node().isEmpty())
			{
				html += itemMask.arg(tr("Uptime:")).arg(secsToString(FClientInfo->lastActivityTime(FContactJid).secsTo(QDateTime::currentDateTime())));
			}
			else if (FContactJid.resource().isEmpty())
			{
				html += itemMask.arg(tr("Inactive:")).arg(secsToString(FClientInfo->lastActivityTime(FContactJid).secsTo(QDateTime::currentDateTime())));
				if (!FClientInfo->lastActivityText(FContactJid).isEmpty())
					html += itemMask.arg(tr("Status:")).arg(Qt::escape(FClientInfo->lastActivityText(FContactJid)));
			}
			else
			{
				html += itemMask.arg(tr("Idle:")).arg(secsToString(FClientInfo->lastActivityTime(FContactJid).secsTo(QDateTime::currentDateTime())));
			}
		}
		else if (!FClientInfo->lastActivityText(FContactJid).isEmpty())
		{
			html += itemMask.arg(tr("Error:")).arg(FClientInfo->lastActivityText(FContactJid));
		}
		else
		{
			html += tr("Waiting response...") + "<br>";
		}
		html += "<br>";
	}

	//Entity Time
	if ((FInfoTypes & IClientInfo::EntityTime)>0)
	{
		html += "<b>" + tr("Entity Time") + "</b><br>";
		if (FClientInfo->hasEntityTime(FContactJid))
		{
			html += itemMask.arg(tr("Time:")).arg(FClientInfo->entityTime(FContactJid).time().toString());
			html += itemMask.arg(tr("Delta:")).arg(secsToString(FClientInfo->entityTimeDelta(FContactJid)));
			html += itemMask.arg(tr("Ping, msec:")).arg(FClientInfo->entityTimePing(FContactJid));
		}
		else if (FClientInfo->entityTimePing(FContactJid) < -1)
		{
			html += tr("Waiting response...") + "<br>";
		}
		else
		{
			html += tr("Not available") + "<br>";
		}
		html += "<br>";
	}

	ui.tedText->setHtml(html);
	this->adjustSize();
}

QString ClientInfoDialog::secsToString(int ASecs) const
{
	const static int secsInMinute = 60;
	const static int secsInHour = 60*secsInMinute;
	const static int secsInDay = 24*secsInHour;
	const static int secsInYear = 365*secsInDay;

	QString time;
	int secs = ASecs;

	int years = secs / secsInYear;
	secs -= years * secsInYear;

	int days = secs / secsInDay;
	secs -= days * secsInDay;

	int hours = secs / secsInHour;
	secs -= hours * secsInHour;

	int minutes = secs / secsInMinute;
	secs -= minutes * secsInMinute;

	if (years > 0)
		time += tr("%1y","years").arg(years) + " ";
	if (days > 0)
		time += tr("%1d","days").arg(days) + " ";
	if (hours > 0)
		time += tr("%1h","hours").arg(hours) + " ";
	if (minutes > 0)
		time += tr("%1m","minutes").arg(minutes) + " ";
	time += tr("%1s","seconds").arg(secs);

	return time;
}

void ClientInfoDialog::onClientInfoChanged(const Jid &AConatctJid)
{
	if (FContactJid == AConatctJid)
		updateText();
}
